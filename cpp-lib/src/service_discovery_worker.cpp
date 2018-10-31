#include "service_discovery_worker.h"
#include "string_utils.h"
#include <cpprest/json.h>
#include <sstream>

using boost::optional;
using std::string;
using std::vector;
using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace pitaya;
namespace json = web::json;

static string ServerAsJson(const Server &server);
static string GetServerKey(const std::string &serverId, const std::string &serverType);

service_discovery::ServiceDiscoveryWorker::ServiceDiscoveryWorker(const string &address, const string &etcdPrefix, pitaya::Server server)
: _server(std::move(server))
, _client(address)
, _watcher(address, _etcdPrefix, std::bind(&ServiceDiscoveryWorker::OnWatch, this, _1))
, _etcdPrefix(etcdPrefix)
, _leaseTTL(60s)
, _leaseKeepAlive(_client)
, _numKeepAliveRetriesLeft(3)
{
    _log = spdlog::get("service_discovery")->clone("service_discovery_worker");
    _log->set_level(spdlog::level::debug);
    _workerThread = std::thread(&ServiceDiscoveryWorker::StartThread, this);
}

service_discovery::ServiceDiscoveryWorker::~ServiceDiscoveryWorker()
{
    {
        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);
        _jobQueue.Clear();
        _semaphore.Notify();
    }

    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    _leaseKeepAlive.Stop();
}

void
service_discovery::ServiceDiscoveryWorker::StartThread()
{
    _log->info("Thread started");

    auto status = Init();

    if (!status.is_ok()) {
        if (status.etcd_error_code == etcdv3::StatusCode::UNDERLYING_GRPC_ERROR) {
            throw PitayaException("gRPC init error: " + status.grpc_error_message);
        } else {
            throw PitayaException("etcd init error: " + status.etcd_error_message);
        }
    }

    for (;;) {
        _semaphore.Wait();

        std::lock_guard<decltype(_jobQueue)> lock(_jobQueue);

        if (_jobQueue.Empty()) {
            _log->info("Exiting loop");
            break;
        }

        JobInfo info = _jobQueue.PopFront();
        switch (info) {
            case JobInfo::EtcdReconnectionFailure:
                _log->error("Reconnection failure, {} retries left!", _numKeepAliveRetriesLeft);

                if (_numKeepAliveRetriesLeft <= 0) {
                    throw std::runtime_error("Failed reconnection to etcd");
                }

                while (_numKeepAliveRetriesLeft > 0) {
                    --_numKeepAliveRetriesLeft;
                    auto status = Bootstrap();
                    if (status.is_ok()) {
                        _numKeepAliveRetriesLeft = 3;
                        break;
                    }
                }

                break;
            case JobInfo::WatchError:
                _log->error("Watch error");
                break;
        }
    }
}

etcdv3::V3Status
service_discovery::ServiceDiscoveryWorker::Init()
{
    auto status = Bootstrap();
    if (!status.is_ok()) {
        return status;
    }

    //
    // TODO: Create a new ticker to manually call SyncServers
    //
    return status;
}

etcdv3::V3Status
service_discovery::ServiceDiscoveryWorker::Bootstrap()
{
    auto status = GrantLease();

    if (!status.is_ok()) {
        return status;
    }

    return BootstrapServer(_server);
}

etcdv3::V3Status
service_discovery::ServiceDiscoveryWorker::GrantLease()
{
    etcd::Response res = _client.leasegrant(_leaseTTL.count()).get();
    if (!res.is_ok()) {
        return res.status;
    }

    _leaseId = res.value.lease_id;
    _log->info("Got lease id: {}", _leaseId);

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
                _semaphore.Notify();
            } break;
        }
    });

    return std::move(res.status);
}

etcdv3::V3Status
service_discovery::ServiceDiscoveryWorker::BootstrapServer(const Server &server)
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
service_discovery::ServiceDiscoveryWorker::AddServerToEtcd(const Server &server)
{
    string key = fmt::format("{}/{}", _etcdPrefix, GetServerKey(server.id, server.type));
    etcd::Response res = _client.set(key, ServerAsJson(server),  _leaseId).get();
    return res.status;
}

void
service_discovery::ServiceDiscoveryWorker::AddServer(const Server &server)
{
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    if (_serversById.Find(server.id) == _serversById.end()) {
        _serversById[server.id] = server;
        _serversByType[server.type][server.id] = server;
    }
}

void
service_discovery::ServiceDiscoveryWorker::SyncServers()
{
    etcd::Response res = _client.ls(_etcdPrefix).get();
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
    PrintServers();
    _lastSyncTime = std::chrono::system_clock::now();
}

optional<pitaya::Server>
service_discovery::ServiceDiscoveryWorker::GetServerFromEtcd(const std::string &serverId, const std::string &serverType)
{
    auto serverKey = fmt::format("{}/{}", _etcdPrefix, GetServerKey(serverId, serverType));

    try {
        etcd::Response res = _client.get(serverKey).get();

        if (!res.is_ok()) {
            string info;
            if (res.status.etcd_error_code == etcdv3::StatusCode::UNDERLYING_GRPC_ERROR) {
                info = res.status.grpc_error_message;
            } else {
                info = res.status.etcd_error_message;
            }
            _log->error("Failed to get key {} from server: {}", serverKey, info);
            return optional<Server>();
        }

        return ParseServer(res.value.value);
    } catch (const std::exception &exc) {
        _log->error("Error in communication with server: {}", exc.what());
        return optional<Server>();
    }
}

void
service_discovery::ServiceDiscoveryWorker::DeleteLocalInvalidServers(const vector<string> &actualServers)
{
    // NOTE(lhahn): Both maps are locked, since we do not want to have them possibly in an inconsistent state.
    std::lock_guard<decltype(_serversById)> serversByIdLock(_serversById);
    std::lock_guard<decltype(_serversByType)> serversByTypeLock(_serversByType);

    for (const auto &pair : _serversById) {
        if (std::find(actualServers.begin(), actualServers.end(), pair.first) == actualServers.end()) {
            _log->warn("Deleting invalid local server {}", pair.first);
            DeleteServer(pair.first);
        }
    }
}


void
service_discovery::ServiceDiscoveryWorker::PrintServers()
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    for (const auto &typePair : _serversByType) {
        _log->info("Type: {}, Servers:", typePair.first);
        for (const auto &svPair : typePair.second) {
            PrintServer(svPair.second);
        }
    }
}

void
service_discovery::ServiceDiscoveryWorker::DeleteServer(const string &serverId)
{
    // NOTE(lhahn): DeleteServer assumes that the resources are already locked.
    if (_serversById.Find(serverId) != _serversById.end()) {
        Server server = _serversById[serverId];
        _serversById.Erase(serverId);
        if (_serversByType.Find(server.type) != _serversByType.end()) {
            auto &serverMap = _serversByType[server.type];
            serverMap.erase(server.id);
        }
    }
}


optional<pitaya::Server>
service_discovery::ServiceDiscoveryWorker::GetServerById(const std::string &id)
{
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    if (_serversById.Find(id) == _serversById.end()) {
        return optional<Server>();
    }

    return optional<Server>(_serversById[id]);
}

std::vector<pitaya::Server>
service_discovery::ServiceDiscoveryWorker::GetServersByType(const std::string &type)
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    vector<Server> servers;

    if (_serversByType.Find(type) != _serversByType.end()) {
        const auto &svMap = _serversByType[type];
        for (const auto &pair : svMap) {
            servers.push_back(pair.second);
        }
    }

    return servers;
}

void
service_discovery::ServiceDiscoveryWorker::OnWatch(etcd::Response res)
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    uint64_t threadId = std::stoull(ss.str());
    _log->info("OnWatch called on thread: {}", threadId);

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
        _log->info("Server {} added", res.value.key);
        PrintServers();
    } else if (res.action == "delete") {
        string serverType, serverId;
        if (!ParseEtcdKey(res.value.key, serverType, serverId)) {
            _log->error("Failed to parse key from etcd: {}", res.value.key);
            return;
        }

        DeleteServer(serverId);
        _log->info("Server {} deleted", serverId);
        PrintServers();
    }
}

static string
GetServerKey(const std::string &serverId, const std::string &serverType)
{
    return fmt::format("{}/{}", serverType, serverId);
}

bool
service_discovery::ServiceDiscoveryWorker::ParseEtcdKey(const string &key, string &serverType, string &serverId)
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

// ======================================================
// Utility functions
// ======================================================

void
service_discovery::ServiceDiscoveryWorker::PrintServer(const Server &server)
{
    _log->debug("Server: {}", GetServerKey(server.id, server.type));
    _log->debug("  - hostname: {}", server.hostname);
    _log->debug("  - frontend: {}", server.frontend);
    _log->debug("  - metadata: {}", server.metadata);
}

static string
ServerAsJson(const Server &server)
{
    json::value obj;
    obj.object();
    obj["id"] = json::value::string(server.id);
    obj["type"] = json::value::string(server.type);
    obj["metadata"] = json::value::string(server.metadata);
    obj["hostname"] = json::value::string(server.hostname);
    obj["frontend"] = json::value::boolean(server.frontend);
    return obj.serialize();
}

optional<Server>
service_discovery::ServiceDiscoveryWorker::ParseServer(const string &jsonStr)
{
    auto jsonSrv = json::value::parse(jsonStr);

    if (!jsonSrv.is_object()) {
        _log->error("Server json is not an object {}", jsonStr);
        return optional<Server>();
    }

    auto server = optional<Server>(Server());

    if (jsonSrv["frontend"].is_boolean()) {
        server->frontend = jsonSrv["frontend"].as_bool();
    }
    if (jsonSrv["type"].is_string()) {
        server->type = jsonSrv["type"].as_string();
    }
    if (jsonSrv["id"].is_string()) {
        server->id = jsonSrv["id"].as_string();
    }
    if (jsonSrv["metadata"].is_string()) {
        server->metadata = jsonSrv["metadata"].as_string();
    }
    if (jsonSrv["hostname"].is_string()) {
        server->hostname = jsonSrv["hostname"].as_string();
    }

    return server;
}

