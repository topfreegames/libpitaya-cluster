#include "pitaya/etcdv3_service_discovery/worker.h"

#include "pitaya/utils.h"
#include "pitaya/utils/string_utils.h"

#include <algorithm>
#include <assert.h>
#include <cpprest/json.h>
#include <sstream>

#ifdef _WIN32
#include <signal.h>
#endif

using boost::optional;
using std::string;
using std::vector;
using std::placeholders::_1;
using namespace pitaya;
namespace json = web::json;

static constexpr const char* kLogTag = "service_discovery_worker";

namespace pitaya {
namespace etcdv3_service_discovery {

static string ServerAsJson(const Server& server);
static string GetServerKey(const std::string& serverId, const std::string& serverType);

Worker::Worker(const EtcdServiceDiscoveryConfig& config,
               pitaya::Server server,
               std::unique_ptr<EtcdClient> etcdClient,
               const char* loggerName)
try
    : _config(config)
    , _server(std::move(server))
    , _etcdClient(std::move(etcdClient))
    , _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _numKeepAliveRetriesLeft(_config.maxNumberOfRetries)
    , _syncServersTicker(config.syncServersIntervalSec, std::bind(&Worker::SyncServers, this)) {

    if (_config.logServerSync) {
        _log->debug("Will synchronize servers every {} seconds",
                    _config.syncServersIntervalSec.count());
    }

    // Print debug information about the filters that will be applied to the etcd keys.
    for (const auto& filter : config.serverTypeFilters) {
        _log->info("Adding server type filter: {}", filter);
    }

    _initPromise = std::make_shared<std::promise<void>>();
    _etcdClient->Watch(std::bind(&Worker::OnWatch, this, _1));
    _workerThread = std::thread(&Worker::StartThread, this);
} catch (const spdlog::spdlog_ex& exc) {
    throw PitayaException(
        fmt::format("Failed to initialize ServiceDiscoveryWorker logger: {}", exc.what()));
}

Worker::~Worker()
{
    _log->debug("Worker Destructor");
    _etcdClient->CancelWatch();

    {
        // Notify the thread about the shutdown.
        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
        _jobQueue.PushBack(Job::NewShutdown());
        _semaphore.Notify();
    }

    _log->debug("Will wait for worker thread");

    // Wait for the worker thread to shutdown first.
    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    _log->debug("Finished waiting for worker thread");
    _log->flush();
}

void
Worker::Shutdown()
{
    _log->info("Shutting down");
    // Make sure that we cancel the watch before destructing the worker thread.
    RevokeLease();
    _etcdClient->StopLeaseKeepAlive();
    _log->debug("Stopping servers ticker");
    _syncServersTicker.Stop();
    _log->debug("Servers ticker stopped");
}

void
Worker::StartThread()
{
    try {

        _log->info("Thread started");
        utils::SetThreadName("NPitSrvcDisWk", _log);

        bool ok = Init();

        if (!ok) {
            _initPromise->set_exception(
                std::make_exception_ptr(PitayaException("Initialization failed")));
            return;
        }

        // Notify main thread that the worker is initialized.
        _initPromise->set_value();

        for (;;) {
            _semaphore.Wait();

            // Get the next job from the queue
            Job job;
            {
                std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
                if (_jobQueue.Empty()) {
                    _log->warn("Job queue empty, ignoring");
                    continue;
                }
                job = _jobQueue.PopFront();
            }

            assert(job.info != JobInfo::Invalid);

            switch (job.info) {
                case JobInfo::SyncServers: {
                    _log->debug("Will synchronize servers");

                    ListResponse res = _etcdClient->List(_config.etcdPrefix + "servers/metagame/");

                    if (!res.ok) {
                        _log->warn("Error synchronizing servers: {}", res.errorMsg);
                        break;
                    }

                    vector<string> allIds;

                    for (size_t i = 0; i < res.keys.size(); ++i) {
                        // 1. Parse key
                        string serverType, serverId;
                        if (!utils::ParseEtcdKey(res.keys[i],
                                                 _config.etcdPrefix,
                                                 _config.serverTypeFilters,
                                                 serverType,
                                                 serverId)) {
                            _log->debug("Ignoring key {}", res.keys[i]);
                            continue;
                        }

                        allIds.push_back(serverId);

                        if (_serversById.FindWithLock(serverId) == _serversById.end()) {
                            _log->info(
                                "Loading info from missing server: {}/{}", serverType, serverId);
                            auto server = GetServerFromEtcd(serverId, serverType);
                            if (!server) {
                                _log->error("Error getting server from etcd: {}", serverId);
                                continue;
                            }

                            AddServer(std::move(server.value()));
                        }
                    }

                    DeleteLocalInvalidServers(std::move(allIds));

                    if (_config.logServerDetails) {
                        PrintServers();
                    }
                    if (_config.logServerSync) {
                        _log->debug("Servers synchronized");
                    }
                    break;
                }
                case JobInfo::Watch: {
                    assert(!job.watchRes.key.empty());
                    assert(!job.watchRes.action.empty());

                    // First we need to parse the etcd key to figure out if it
                    // belongs to the same prefix and it is actually a server.
                    string serverType, serverId;
                    if (!utils::ParseEtcdKey(job.watchRes.key,
                                             _config.etcdPrefix,
                                             _config.serverTypeFilters,
                                             serverType,
                                             serverId)) {
                        _log->debug("Watch: Ignoring {}", job.watchRes.key);
                        break;
                    }

                    if (job.watchRes.action == "create") {
                        _log->debug("Watch: received create action for server {}",
                                    job.watchRes.key);
                        auto server = ParseServer(job.watchRes.value);
                        if (!server) {
                            _log->error("Watch: Error parsing server: {}", job.watchRes.value);
                            break;
                        }
                        AddServer(std::move(server.value()));
                    } else if (job.watchRes.action == "delete") {
                        _log->debug("Watch: received delete action for server {}",
                                    job.watchRes.key);
                        DeleteServer(serverId);
                    }

                    PrintServers();
                    break;
                }
                case JobInfo::AddListener: {
                    _log->debug("Adding listener");
                    assert(job.listener && "listener should not be null");
                    // Whenever we add a new listener, we want to call ServerAdded
                    // for all existent servers on the class.
                    {
                        std::lock_guard<decltype(_serversById)> lock(_serversById);
                        for (const auto& pair : _serversById) {
                            const Server& server = pair.second;
                            _log->debug("Broadcasting server added to the listener for id {}",
                                        server.Id());
                            job.listener->ServerAdded(server);
                        }
                    }

                    std::lock_guard<decltype(_listeners)> lock(_listeners);
                    _listeners.PushBack(job.listener);
                    break;
                }
                case JobInfo::EtcdReconnectionFailure: {
                    _log->error("Reconnection failure, {} retries left out of {}!",
                                _numKeepAliveRetriesLeft,
                                _config.maxNumberOfRetries);
                    _etcdClient->StopLeaseKeepAlive();
                    _syncServersTicker.Stop();

                    while (_numKeepAliveRetriesLeft > 0) {
                        _log->info("ETCD retries left: {}", _numKeepAliveRetriesLeft);
                        auto delay_milliseconds =
                            _config.retryDelayMilliseconds
                            << (_config.maxNumberOfRetries - _numKeepAliveRetriesLeft);
                        _log->info("ETCD retry waiting for {}ms", delay_milliseconds);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_milliseconds));

                        --_numKeepAliveRetriesLeft;
                        auto ok = Bootstrap();

                        if (ok) {
                            _log->info("Etcd reconnection successful");
                            _numKeepAliveRetriesLeft = _config.maxNumberOfRetries;
                            _log->info("Restarting etcd watcher");
                            _etcdClient->Watch(std::bind(&Worker::OnWatch, this, _1));
                            StartLeaseKeepAlive();
                            _syncServersTicker.Start();
                            break;
                        }
                    }

                    if (_numKeepAliveRetriesLeft <= 0) {
                        _log->critical("Failed to reconnect to etcd, shutting down");
                        Shutdown();
                        std::thread(std::bind(raise, SIGTERM)).detach();
                        _log->debug("Exiting loop");
                        return;
                    }

                    break;
                }
                case JobInfo::WatchError: {
                    _log->error("Watch error");
                    break;
                }
                case JobInfo::Shutdown: {
                    Shutdown();
                    _log->debug("Exiting loop");
                    return;
                }
                default: {
                    _log->error("This code should be unreachable");
                    assert(false);
                }
            }
        }

        _log->debug("Thread exited loop");
    } catch (const std::exception& e) {
        _log->error("Error in worker thread: {}", e.what());
        _initPromise->set_exception(std::make_exception_ptr(e));
        return;
    }
}

void
Worker::StartLeaseKeepAlive()
{
    if (_leaseId == -1) {
        _log->error("LeaseId is -1, cannot start lease keep alive");
        return;
    }

    std::function<void(std::exception_ptr)> handler = [this](std::exception_ptr exc) {
        try {
            if (exc) {
                _log->error("lease keep alive failed!");
                std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
                _jobQueue.PushBack(Job::NewEtcdReconnectionFailure());
                _syncServersTicker.Stop();
                _semaphore.Notify();
                std::rethrow_exception(exc);
            }
        } catch (const std::runtime_error& e) {
            _log->error("ETCD Connection failure: {}", e.what());
        } catch (const std::out_of_range& e) {
            _log->error("ETCD Lease expired: {}", e.what());
        }
    };
    _etcdClient->LeaseKeepAlive(_config.heartbeatTTLSec.count(), _leaseId, handler);
}

bool
Worker::Init()
{
    bool ok = Bootstrap();
    if (!ok) {
        _log->error("Service discovery bootstrap failed");
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
        _log->error("Failed to add server to etcd");
        return false;
    }

    SyncServers();
    return true;
}

bool
Worker::AddServerToEtcd(const Server& server)
{
    _log->info("Adding server to etcd with lease {}", _leaseId);
    string key = fmt::format("{}{}", _config.etcdPrefix, GetServerKey(server.Id(), server.Type()));
    SetResponse res = _etcdClient->Set(key, ServerAsJson(server), _leaseId);
    return res.ok;
}

void
Worker::AddServer(const Server& server)
{
    {
        std::lock_guard<decltype(_serversById)> lock(_serversById);

        if (_serversById.Find(server.Id()) != _serversById.end()) {
            return;
        }

        _log->debug("Adding server {} with metadata {} to service_discovery",
                    server.Id(),
                    server.Metadata());
        _serversById[server.Id()] = server;
        _serversByType[server.Type()][server.Id()] = server;
    }

    BroadcastServerAdded(server);
}

void
Worker::SyncServers()
{
    std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
    _jobQueue.PushBack(Job::NewSyncServers());
    _semaphore.Notify();
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
        _log->warn("Invalid local server {}, removing from server list", invalidServer);
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

    if (_serversByType.Find(server.Type()) != _serversByType.end()) {
        auto& serverMap = _serversByType[server.Type()];
        serverMap.erase(server.Id());
    }

    _log->debug("Server {} deleted", server.Id());
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
    std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
    if (res.ok) {
        _jobQueue.PushBack(Job::NewWatch(res));
        _semaphore.Notify();
    } else {
        _jobQueue.PushBack(Job::NewWatchError());
        _semaphore.Notify();
    }
}

void
Worker::WaitUntilInitialized()
{
    _initPromise->get_future().wait();
}

// ======================================================
// Utility functions
// ======================================================

static string
GetServerKey(const std::string& serverId, const std::string& serverType)
{
    return fmt::format("servers/{}/{}", serverType, serverId);
}

void
Worker::PrintServer(const Server& server)
{
    bool printServerDetails = false;
    _log->debug("  Server: {}", GetServerKey(server.Id(), server.Type()));
    if (printServerDetails) {
        _log->debug("    - hostname: {}", server.Hostname());
        _log->debug("    - frontend: {}", server.IsFrontend());
        _log->debug("    - metadata: {}", server.Metadata());
    }
}

static string
ServerAsJson(const Server& server)
{
    json::value obj;
    obj.object();
    obj[U("id")] = json::value::string(utility::conversions::to_string_t(server.Id()));
    obj[U("type")] = json::value::string(utility::conversions::to_string_t(server.Type()));
    try {
        obj[U("metadata")] = json::value::parse(server.Metadata());
    } catch (const json::json_exception& exc) {
        obj[U("metadata")] = json::value::string(U(""));
    }
    obj[U("hostname")] = json::value::string(utility::conversions::to_string_t(server.Hostname()));
    obj[U("frontend")] = json::value::boolean(server.IsFrontend());
    return utility::conversions::to_utf8string(obj.serialize());
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

        bool frontend = false;
        std::string hostname, type, id, metadata;

        if (jsonSrv.has_boolean_field(U("frontend"))) {
            frontend = jsonSrv[U("frontend")].as_bool();
        }
        if (jsonSrv.has_string_field(U("type"))) {
            type = utility::conversions::to_utf8string(jsonSrv[U("type")].as_string());
        }
        if (jsonSrv.has_string_field(U("id"))) {
            id = utility::conversions::to_utf8string(jsonSrv[U("id")].as_string());
        }
        if (jsonSrv.has_object_field(U("metadata"))) {
            metadata = utility::conversions::to_utf8string(jsonSrv[U("metadata")].serialize());
        }
        if (jsonSrv.has_string_field(U("hostname"))) {
            hostname = utility::conversions::to_utf8string(jsonSrv[U("hostname")].as_string());
        }

        if (id.empty() || type.empty()) {
            return boost::none;
        }

        auto sv = Server((Server::Kind)frontend, id, type, hostname).WithRawMetadata(metadata);

        return sv;
    } catch (const json::json_exception& exc) {
        _log->error("Failed to parse server json ({}): {}", jsonStr, exc.what());
        return boost::none;
    }
}

void
Worker::AddListener(service_discovery::Listener* listener)
{
    assert(listener && "listener should not be null");
    std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
    _jobQueue.PushBack(Job::NewAddListener(listener));
    _semaphore.Notify();
}

void
Worker::RemoveListener(service_discovery::Listener* listener)
{
    std::lock_guard<decltype(_listeners)> lock(_listeners);
    if (std::find(_listeners.begin(), _listeners.end(), listener) != _listeners.end()) {
        _listeners.Erase(std::remove(_listeners.begin(), _listeners.end(), listener),
                         _listeners.end());
    }
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
