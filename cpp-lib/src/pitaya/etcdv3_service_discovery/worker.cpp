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

Worker::Worker(const Config& config, pitaya::Server server, const char* loggerName) try
    : _config(config)
    , _workerExiting(false)
    , _server(std::move(server))
    , _client(config.endpoints)
    , _watcher(config.endpoints, config.etcdPrefix, std::bind(&Worker::OnWatch, this, _1))
    , _log(spdlog::get(loggerName)->clone("service_discovery_worker"))
    , _leaseKeepAlive(_client, config.logHeartbeat, loggerName)
    , _numKeepAliveRetriesLeft(3)
    , _syncServersTicker(config.syncServersIntervalSec, std::bind(&Worker::SyncServers, this)) {

    if (_config.logServerSync) {
        _log->debug("Will synchronize servers every {} seconds",
                    _config.syncServersIntervalSec.count());
    }
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
    _leaseKeepAlive.Stop();
    _log->debug("Stopping servers ticker");
    _syncServersTicker.Stop();
    _log->debug("Servers ticker stopped");
}

void
Worker::StartThread()
{
    _log->info("Thread started");

    auto status = Init();

    if (!status.is_ok()) {
        if (status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR) {
            throw PitayaException("gRPC init error: " + status.grpc_error_message);
        } else {
            throw PitayaException("etcd init error: " + status.etcd_error_message);
        }
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
                _client.stop_lease_keep_alive();
                _syncServersTicker.Stop();

                while (_numKeepAliveRetriesLeft > 0) {
                    --_numKeepAliveRetriesLeft;
                    auto status = Bootstrap();
                    if (status.is_ok()) {
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

    _leaseKeepAlive.SetLeaseId(_leaseId);
    _leaseKeepAlive.Start().then([this](LeaseKeepAliveStatus status) {
        switch (status) {
            case LeaseKeepAliveStatus::Ok:
                _log->info("lease keep alive exited with success");
                break;
            case LeaseKeepAliveStatus::Fail: {
                _log->error("lease keep alive failed!");
                std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
                _jobQueue.PushBack(JobInfo::EtcdReconnectionFailure);
                _syncServersTicker.Stop();
                _semaphore.Notify();
            } break;
        }
    });
}

etcdv3::V3Status
Worker::Init()
{
    auto status = Bootstrap();
    if (!status.is_ok()) {
        return status;
    }

    StartLeaseKeepAlive();
    _syncServersTicker.Start();

    return status;
}

etcdv3::V3Status
Worker::Bootstrap()
{
    auto status = GrantLease();

    if (!status.is_ok()) {
        return status;
    }

    return BootstrapServer(_server);
}

etcdv3::V3Status
Worker::GrantLease()
{
    etcd::Response res = _client.leasegrant(_config.heartbeatTTLSec.count()).get();
    if (!res.is_ok()) {
        return res.status;
    }

    _leaseId = res.value.lease_id;
    _log->info("Got lease id: {}", _leaseId);

    return std::move(res.status);
}

etcdv3::V3Status
Worker::BootstrapServer(const Server& server)
{
    etcdv3::V3Status status;

    status = AddServerToEtcd(server);
    if (!status.is_ok()) {
        return status;
    }

    SyncServers();

    return status;
}

etcdv3::V3Status
Worker::AddServerToEtcd(const Server& server)
{
    string key = fmt::format("{}{}", _config.etcdPrefix, GetServerKey(server.id, server.type));
    etcd::Response res = _client.set(key, ServerAsJson(server), _leaseId).get();
    return res.status;
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

    etcd::Response res = _client.ls(_config.etcdPrefix).get();
    if (!res.is_ok()) {
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
        etcd::Response res = _client.get(serverKey).get();

        if (!res.is_ok()) {
            string info;
            if (res.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR) {
                info = res.status.grpc_error_message;
            } else {
                info = res.status.etcd_error_message;
            }
            _log->error("Failed to get key {} from server: {}", serverKey, info);
            return boost::none;
        }

        return ParseServer(res.value.value);
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
    etcd::Response res = _client.lease_revoke(_leaseId).get();
    if (res.is_ok()) {
        _log->info("Lease revoked successfuly");
    } else {
        _log->error("Failed to revoke lease: etcd = {}, grpc = {}",
                    res.status.etcd_error_message,
                    res.status.grpc_error_message);
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
Worker::OnWatch(etcd::Response res)
{
    if (_workerExiting) {
        _log->debug("Worker is exiting, ignoring OnWatch call");
        return;
    }

    if (!res.is_ok()) {
        _log->error("OnWatch error");
        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
        _jobQueue.PushBack(JobInfo::WatchError);
        _semaphore.Notify();
        return;
    }

    if (res.action == "create") {
        auto server = ParseServer(res.value.value);
        if (!server) {
            _log->error("Error parsing server: {}", res.value.value);
            return;
        }

        AddServer(std::move(server.value()));
        PrintServers();
    } else if (res.action == "delete") {
        string serverType, serverId;
        if (!ParseEtcdKey(res.value.key, serverType, serverId)) {
            _log->error("Failed to parse key from etcd: {}", res.value.key);
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
