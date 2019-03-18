#include "pitaya/etcdv3_service_discovery/worker.h"

#include "pitaya/utils/string_utils.h"

#include <assert.h>
#include <cpprest/json.h>
#include <sstream>

using boost::optional;
using std::string;
using std::vector;
using std::placeholders::_1;
using namespace pitaya;
namespace json = web::json;

namespace pitaya {
namespace etcdv3_service_discovery {

static string ServerAsJson(const Server& server);
static string GetServerKey(const std::string& serverId, const std::string& serverType);

Worker::Worker(const Config& config,
               pitaya::Server server,
               std::unique_ptr<EtcdClient> etcdClient,
               const char* loggerName) try
    : _config(config)
    , _workerExiting(false)
    , _server(std::move(server))
    , _etcdClient(std::move(etcdClient))
    , _log(spdlog::get(loggerName)->clone("service_discovery_worker"))
    , _numKeepAliveRetriesLeft(3)
    , _syncServersTicker(config.syncServersIntervalSec, std::bind(&Worker::SyncServers, this)) {

    if (_config.logServerSync) {
        _log->debug("Will synchronize servers every {} seconds",
                    _config.syncServersIntervalSec.count());
    }
    _etcdClient->Watch(std::bind(&Worker::OnWatch, this, _1));
    _workerThread = std::thread(&Worker::StartThread, this);
} catch (const etcd::watch_error& exc) {
    throw PitayaException(
        fmt::format("Failed to initialize ServiceDiscovery watcher: {}", exc.what()));
} catch (const spdlog::spdlog_ex& exc) {
    throw PitayaException(
        fmt::format("Failed to initialize ServiceDiscovery watcher: {}", exc.what()));
}

Worker::~Worker()
{
    _log->debug("Worker Destructor");

    {
        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
        _jobQueue.PushBack(JobInfo::Shutdown);
        _semaphore.Notify();
    }

    _log->debug("Will wait for worker thread");

    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    _log->debug("Finished waiting for worker thread");

    _log->flush();
    spdlog::drop("service_disovery_worker");
}

void
Worker::Shutdown()
{
    _log->info("Shutting down");
    _workerExiting = true;
    _etcdClient->StopLeaseKeepAlive();
    _log->debug("Stopping servers ticker");
    _syncServersTicker.Stop();
    _log->debug("Servers ticker stopped");
}

void
Worker::StartThread()
{
    _log->info("Thread started");

    bool ok = Init();

    if (!ok) {
        throw PitayaException("Initialization failed");
    }

    // Notify main thread that the worker is initialized.
    _initPromise.set_value();

    for (;;) {
        _semaphore.Wait();

        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);

        if (_jobQueue.Empty()) {
            _log->warn("Job queue empty, ignoring");
            continue;
        }

        JobInfo info = _jobQueue.PopFront();
        switch (info) {
            case JobInfo::EtcdReconnectionFailure:
                _log->error("Reconnection failure, {} retries left!", _numKeepAliveRetriesLeft);
                _etcdClient->StopLeaseKeepAlive();
                _syncServersTicker.Stop();

                while (_numKeepAliveRetriesLeft > 0) {
                    --_numKeepAliveRetriesLeft;
                    auto ok = Bootstrap();
                    if (ok) {
                        _log->info("Etcd reconnection successful");
                        _numKeepAliveRetriesLeft = 3;
                        StartLeaseKeepAlive();
                        break;
                    }
                }

                if (_numKeepAliveRetriesLeft <= 0) {
                    _log->critical("Failed to reconnecto to etcd, shutting down");
                    Shutdown();
                    std::thread(std::bind(raise, SIGTERM)).detach();
                    _log->debug("Exiting loop");
                    return;
                }

                break;
            case JobInfo::Shutdown:
                _log->info("Shutting down");
                RevokeLease();
                Shutdown();
                _log->debug("Exiting loop");
                return;
            case JobInfo::WatchError:
                _log->error("Watch error");
                break;
        }
    }

    _log->debug("Thread exited loop");
}

void
Worker::StartLeaseKeepAlive()
{
    if (_leaseId == -1) {
        _log->error("LeaseId is -1, cannot start lease keep alive");
        return;
    }

    _etcdClient->LeaseKeepAlive(_leaseId, [this](EtcdLeaseKeepAliveStatus status) {
        switch (status) {
            case EtcdLeaseKeepAliveStatus::Ok:
                _log->info("lease keep alive exited with success");
                break;
            case EtcdLeaseKeepAliveStatus::Fail: {
                _log->error("lease keep alive failed!");
                std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
                _jobQueue.PushBack(JobInfo::EtcdReconnectionFailure);
                _syncServersTicker.Stop();
                _semaphore.Notify();
            } break;
        }
    });
}

bool
Worker::Init()
{
    bool ok = Bootstrap();
    if (!ok) {
        return false;
    }

    StartLeaseKeepAlive();
    _syncServersTicker.Start();
    return true;
}

bool
Worker::Bootstrap()
{
    LeaseGrantResponse res = _etcdClient->LeaseGrant(_config.heartbeatTTLSec);

    if (!res.ok) {
        _log->error("Lease grant failed: " + res.errorMsg);
        return false;
    }

    _leaseId = res.leaseId;
    _log->info("Got lease id: {}", _leaseId);

    // Now bootstrap the server by adding itself to the servers in etcd
    // and synchronizing the remote servers locally.
    auto ok = AddServerToEtcd(_server);
    if (!ok) {
        return false;
    }

    SyncServers();
    return true;
}

bool
Worker::AddServerToEtcd(const Server& server)
{
    string key = fmt::format("{}{}", _config.etcdPrefix, GetServerKey(server.id, server.type));
    SetResponse res = _etcdClient->Set(key, ServerAsJson(server), _leaseId);
    return res.ok;
}

void
Worker::AddServer(const Server& server)
{
    {
        std::lock_guard<decltype(_serversById)> lock(_serversById);

        if (_serversById.Find(server.id) != _serversById.end()) {
            return;
        }

        _log->debug(
            "Adding server {} with metadata {} to service_discovery", server.id, server.metadata);
        _serversById[server.id] = server;
        _serversByType[server.type][server.id] = server;
    }

    BroadcastServerAdded(server);
}

void
Worker::SyncServers()
{
    if (_workerExiting) {
        _log->info("Worker exiting, no need to synchronize servers");
        return;
    }

    if (_config.logServerSync)
        _log->debug("Will synchronize servers");

    ListResponse res = _etcdClient->List(_config.etcdPrefix);
    if (!res.ok) {
        _log->error("Error synchronizing servers");
        return;
    }

    vector<string> allIds;

    for (size_t i = 0; i < res.keys.size(); ++i) {
        // 1. Parse key
        string serverType, serverId;
        bool ok = ParseEtcdKey(res.keys[i], serverType, serverId);
        if (!ok) {
            _log->error("Failed to parse etcd key {}", res.keys[i]);
        }

        allIds.push_back(serverId);

        if (_serversById.FindWithLock(serverId) == _serversById.end()) {
            _log->info("Loading info from missing server: {}/{}", serverType, serverId);
            auto server = GetServerFromEtcd(serverId, serverType);
            if (!server) {
                _log->error("Error getting server from etcd: {}", serverId);
                continue;
            }

            AddServer(std::move(server.value()));
        }
    }

    DeleteLocalInvalidServers(std::move(allIds));

    if (_config.logServerDetails)
        PrintServers();
    if (_config.logServerSync)
        _log->debug("Servers synchronized");
}

optional<pitaya::Server>
Worker::GetServerFromEtcd(const std::string& serverId, const std::string& serverType)
{
    auto serverKey = fmt::format("{}{}", _config.etcdPrefix, GetServerKey(serverId, serverType));

    try {
        GetResponse res = _etcdClient->Get(serverKey);

        if (!res.ok) {
            _log->error("Failed to get key {} from server: {}", serverKey, res.errorMsg);
            return boost::none;
        }

        return ParseServer(res.value);
    } catch (const std::exception& exc) {
        _log->error("Error in communication with server: {}", exc.what());
        return boost::none;
    }
}

void
Worker::DeleteLocalInvalidServers(const vector<string>& actualServers)
{
    // NOTE(lhahn): Both maps are locked, since we do not want to have them
    // possibly in an inconsistent state.
    std::lock_guard<decltype(_serversById)> serversByIdLock(_serversById);
    std::lock_guard<decltype(_serversByType)> serversByTypeLock(_serversByType);

    std::vector<std::string> invalidServers;

    for (const auto& pair : _serversById) {
        if (std::find(actualServers.begin(), actualServers.end(), pair.first) ==
            actualServers.end()) {
            invalidServers.push_back(pair.first);
        }
    }

    for (const auto& invalidServer : invalidServers) {
        _log->warn("Deleting invalid local server {}", invalidServer);
        DeleteServer(invalidServer);
    }
}

void
Worker::RevokeLease()
{
    _log->info("Revoking lease");
    LeaseRevokeResponse res = _etcdClient->LeaseRevoke(_leaseId);
    if (res.ok) {
        _log->info("Lease revoked successfuly");
    } else {
        _log->error("Failed to revoke lease: {}", res.errorMsg);
    }
}

void
Worker::PrintServers()
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    for (const auto& typePair : _serversByType) {
        _log->debug("Type: {}, Servers:", typePair.first);
        for (const auto& svPair : typePair.second) {
            PrintServer(svPair.second);
        }
    }
}

void
Worker::DeleteServer(const string& serverId)
{
    // NOTE(lhahn): DeleteServer assumes that the resources are already locked.
    if (_serversById.Find(serverId) == _serversById.end()) {
        return;
    }

    Server server = _serversById[serverId];
    _serversById.Erase(serverId);

    if (_serversByType.Find(server.type) != _serversByType.end()) {
        auto& serverMap = _serversByType[server.type];
        serverMap.erase(server.id);
    }

    BroadcastServerRemoved(server);
}

optional<pitaya::Server>
Worker::GetServerById(const std::string& id)
{
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    if (_serversById.Find(id) == _serversById.end()) {
        return optional<Server>();
    }

    return optional<Server>(_serversById[id]);
}

std::vector<pitaya::Server>
Worker::GetServersByType(const std::string& type)
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    vector<Server> servers;

    if (_serversByType.Find(type) != _serversByType.end()) {
        const auto& svMap = _serversByType[type];
        for (const auto& pair : svMap) {
            servers.push_back(pair.second);
        }
    }

    return servers;
}

void
Worker::OnWatch(WatchResponse res)
{
    if (_workerExiting) {
        _log->debug("Worker is exiting, ignoring OnWatch call");
        return;
    }

    if (!res.ok) {
        _log->error("OnWatch error");
        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
        _jobQueue.PushBack(JobInfo::WatchError);
        _semaphore.Notify();
        return;
    }

    if (res.action == "create") {
        auto server = ParseServer(res.value);
        if (!server) {
            _log->error("Error parsing server: {}", res.value);
            return;
        }

        AddServer(std::move(server.value()));
        PrintServers();
    } else if (res.action == "delete") {
        string serverType, serverId;
        if (!ParseEtcdKey(res.key, serverType, serverId)) {
            _log->error("Failed to parse key from etcd: {}", res.key);
            return;
        }

        DeleteServer(serverId);
        _log->debug("Server {} deleted", serverId);
        PrintServers();
    }
}

void
Worker::WaitUntilInitialized()
{
    _initPromise.get_future().wait();
}

// ======================================================
// Utility functions
// ======================================================

bool
Worker::ParseEtcdKey(const string& key, string& serverType, string& serverId)
{
    auto comps = string_utils::Split(key, '/');
    if (comps.size() != 4) {
        _log->error("Error parsing etcd key {} (server name can't contain /)", key);
        return false;
    }

    serverType = comps[2];
    serverId = comps[3];
    return true;
}

static string
GetServerKey(const std::string& serverId, const std::string& serverType)
{
    return fmt::format("servers/{}/{}", serverType, serverId);
}

void
Worker::PrintServer(const Server& server)
{
    bool printServerDetails = false;
    _log->debug("  Server: {}", GetServerKey(server.id, server.type));
    if (printServerDetails) {
        _log->debug("    - hostname: {}", server.hostname);
        _log->debug("    - frontend: {}", server.frontend);
        _log->debug("    - metadata: {}", server.metadata);
    }
}

static string
ServerAsJson(const Server& server)
{
    json::value obj;
    obj.object();
    obj["id"] = json::value::string(server.id);
    obj["type"] = json::value::string(server.type);
    try {
        obj["metadata"] = json::value::parse(server.metadata);
    } catch (const json::json_exception& exc) {
        obj["metadata"] = json::value::string("");
    }
    obj["hostname"] = json::value::string(server.hostname);
    obj["frontend"] = json::value::boolean(server.frontend);
    return obj.serialize();
}

optional<Server>
Worker::ParseServer(const string& jsonStr)
{
    try {
        auto jsonSrv = json::value::parse(jsonStr);

        if (!jsonSrv.is_object()) {
            _log->error("Server json is not an object {}", jsonStr);
            return boost::none;
        }

        auto server = optional<Server>(Server());

        if (jsonSrv.has_boolean_field("frontend")) {
            server->frontend = jsonSrv["frontend"].as_bool();
        }
        if (jsonSrv.has_string_field("type")) {
            server->type = jsonSrv["type"].as_string();
        }
        if (jsonSrv.has_string_field("id")) {
            server->id = jsonSrv["id"].as_string();
        }
        if (jsonSrv.has_object_field("metadata")) {
            server->metadata = jsonSrv["metadata"].serialize();
        }
        if (jsonSrv.has_string_field("hostname")) {
            server->hostname = jsonSrv["hostname"].as_string();
        }

        return server;
    } catch (const json::json_exception& exc) {
        _log->error("Failed to parse server json ({}): {}", jsonStr, exc.what());
        return boost::none;
    }
}

void
Worker::AddListener(service_discovery::Listener* listener)
{
    // Whenever we add a new listener, we want to call ServerAdded
    // for all existent servers on the class.
    for (const auto& pair : _serversById) {
        const Server& server = pair.second;
        listener->ServerAdded(server);
    }

    std::lock_guard<decltype(_listeners)> lock(_listeners);
    _listeners.PushBack(listener);
}

void
Worker::BroadcastServerAdded(const pitaya::Server& server)
{
    std::lock_guard<decltype(_listeners)> lock(_listeners);
    for (auto l : _listeners) {
        if (l) {
            l->ServerAdded(server);
        }
    }
}

void
Worker::BroadcastServerRemoved(const pitaya::Server& server)
{
    std::lock_guard<decltype(_listeners)> lock(_listeners);
    for (auto l : _listeners) {
        if (l) {
            l->ServerRemoved(server);
        }
    }
}

} // namespace etcdv3_service_discovery
} // namespace pitaya
