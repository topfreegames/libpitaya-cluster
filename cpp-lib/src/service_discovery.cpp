#include "service_discovery.h"
#include <cpprest/json.h>
#include "string_utils.h"
#include <algorithm>
#include "spdlog/sinks/stdout_color_sinks.h"

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::cout;
using std::cerr;
using std::endl;
using std::chrono::seconds;
namespace chrono = std::chrono;
namespace json = web::json;

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace pitaya;

// Helper functions
static void PrintServer(const Server &server);
static string ServerAsJson(const Server &server);
static string GetServerKey(const string &serverId, const string &serverType);

service_discovery::ServiceDiscovery::ServiceDiscovery(shared_ptr<Server> server, const string &address)
: _log(spdlog::stdout_color_mt("service_discovery"))
, _server(std::move(server))
, _client(address)
, _etcdPrefix("pitaya/servers")
, _watcher(address, _etcdPrefix, std::bind(&ServiceDiscovery::OnWatch, this, _1))
, _heartbeatTTL(60s)
, _leaseId(0)
{
    Configure();
    etcdv3::V3Status status = Init();
    if (!status.is_ok()) {
        if (status.etcd_error_code == etcdv3::StatusCode::UNDERLYING_GRPC_ERROR) {
            throw PitayaException("gRPC init error: " + status.grpc_error_message);
        } else {
            throw PitayaException("etcd init error: " + status.etcd_error_message);
        }
    }
}

etcdv3::V3Status
service_discovery::ServiceDiscovery::Init()
{
    _running = true;

    etcdv3::V3Status status;

    status = Bootstrap();
    if (!status.is_ok()) {
        return status;
    }

    //
    // TODO: Create a new ticker to manually call SyncServers
    //
    return status;
}

void
service_discovery::ServiceDiscovery::OnWatch(etcd::Response res)
{
    if (!res.is_ok()) {
        _log->error("OnWatch error");
        return;
    }

    if (res.action == "create") {
        auto server = ParseServer(res.value.value);
        if (server == nullptr) {
            _log->error("Error parsing server: {}", res.value.value);
            return;
        }

        AddServer(std::move(server));
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

    cout << "Watch\n";
    cout << "Key: " << res.value.key << "\n";
    cout << "val: " << res.value.value << "\n";
    cout << "action: " << res.action << "\n";
}

int64_t
service_discovery::ServiceDiscovery::CreateLease()
{
    etcd::Response res = _client.leasegrant(_heartbeatTTL.count()).get();

    if (!res.is_ok()) {
        _log->error("leasegrant() failed!");
        return -1;
    }

    // TODO: Keep lease alive by renewing the lease every X seconds (do this in a thread or async channel)
    return res.value.lease_id;
}

etcdv3::V3Status
service_discovery::ServiceDiscovery::Bootstrap()
{
    etcdv3::V3Status status;

    status = GrantLease();
    if (!status.is_ok()) {
        return status;
    }

    return BootstrapServer(*_server.get());
}

etcdv3::V3Status
service_discovery::ServiceDiscovery::GrantLease()
{
    etcd::Response res = _client.leasegrant(_heartbeatTTL.count()).get();
    if (!res.is_ok()) {
        return res.status;
    }

    _leaseId = res.value.lease_id;
    _log->info("Got lease id: {}", _leaseId);

    //
    // TODO, FIXME: Add KeepAlive here
    //

    return std::move(res.status);
}

etcdv3::V3Status
service_discovery::ServiceDiscovery::BootstrapServer(const Server &server)
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
service_discovery::ServiceDiscovery::AddServerToEtcd(const Server &server)
{
    string key = fmt::format("{}/{}", _etcdPrefix, GetServerKey(server.id, server.type));
    etcd::Response res = _client.set(key, ServerAsJson(server),  _leaseId).get();
    return res.status;
}

void
service_discovery::ServiceDiscovery::AddServer(shared_ptr<Server> server)
{
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    if (_serversById[server->id] == nullptr) {
        _serversById[server->id] = server;
        _serversByType[server->type][server->id] = server;
    }
}

shared_ptr<pitaya::Server>
service_discovery::ServiceDiscovery::GetServerById(const string &id)
{
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    return _serversById[id];
}

vector<shared_ptr<Server>>
service_discovery::ServiceDiscovery::GetServersByType(const std::string &type)
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    vector<shared_ptr<Server>> servers;

    if (_serversByType.Find(type) != _serversByType.end()) {
        const auto &svMap = _serversByType[type];
        for (const auto &pair : svMap) {
            servers.push_back(pair.second);
        }
    }

    return servers;
}

shared_ptr<Server>
service_discovery::ServiceDiscovery::GetServerFromEtcd(const std::string &serverId, const std::string &serverType)
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
            return nullptr;
        }

        return ParseServer(res.value.value);
    } catch (const std::exception &exc) {
        _log->error("Error in communication with server: {}", exc.what());
        return nullptr;
    }
}

void
service_discovery::ServiceDiscovery::SyncServers()
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

            AddServer(std::move(server));
        }
    }

    DeleteLocalInvalidServers(std::move(allIds));

    PrintServers();
    _lastSyncTime = chrono::system_clock::now();
}

void
service_discovery::ServiceDiscovery::PrintServers()
{
    std::lock_guard<decltype(_serversByType)> lock(_serversByType);
    for (const auto &typePair : _serversByType) {
        _log->info("Type: {}, Servers:", typePair.first);
        for (const auto &svPair : typePair.second) {
            PrintServer(*svPair.second.get());
        }
    }
}

void
service_discovery::ServiceDiscovery::DeleteServer(const string &serverId)
{
    // NOTE(lhahn): DeleteServer assumes that the resources are already locked.
    if (_serversById.Find(serverId) != _serversById.end()) {
        shared_ptr<Server> server = _serversById[serverId];
        _serversById.Erase(serverId);
        if (_serversByType.Find(server->type) != _serversByType.end()) {
            auto &serverMap = _serversByType[server->type];
            serverMap.erase(server->id);
        }
    }
}

void
service_discovery::ServiceDiscovery::DeleteLocalInvalidServers(const vector<string> &actualServers)
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
service_discovery::ServiceDiscovery::Configure()
{
    _syncServersInterval = 2s;
    _heartbeatTTL = 1s;
    _logHeartbeat = true;
//    _etcdDialTimeout = seconds{5};
    _revokeTimeout = 10s;
    _grantLeaseTimeout = 60s;
    _grantLeaseMaxRetries = 4;
    _grantLeaseInterval = seconds{_grantLeaseTimeout/3};
}

// ======================================================
// Utility functions
// ======================================================

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

shared_ptr<pitaya::Server>
service_discovery::ServiceDiscovery::ParseServer(const string &jsonStr)
{
    auto server = std::make_shared<pitaya::Server>();

    auto jsonSrv = json::value::parse(jsonStr);

    if (!jsonSrv.is_object()) {
        _log->error("Server json is not an object {}", jsonStr);
        return nullptr;
    }

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

static string
GetServerKey(const string &serverId, const string &serverType)
{
    return fmt::format("{}/{}", serverType, serverId);
}

bool
service_discovery::ServiceDiscovery::ParseEtcdKey(const string &key, string &serverType, string &serverId)
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

static void
PrintServer(const Server &server)
{
    cout << "Server: " << GetServerKey(server.id, server.type) << "\n";
    cout << "  - hostname: " << server.hostname << "\n";
    cout << "  - frontend: " << server.frontend << "\n";
    cout << "  - metadata: " << server.metadata << "\n";
}

