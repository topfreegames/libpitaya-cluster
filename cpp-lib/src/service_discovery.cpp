#include "service_discovery.h"
#include <cpprest/json.h>
#include "string_utils.h"

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::cout;
using std::cerr;
using std::endl;
using std::chrono::seconds;
namespace json = web::json;

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace pitaya;

// Helper functions
static string ServerAsJson(const Server &server);
static std::shared_ptr<pitaya::Server> ParseServer(const string &jsonStr);
static string GetServerKey(const string &serverId, const string &serverType);

service_discovery::ServiceDiscovery::ServiceDiscovery(shared_ptr<Server> server, const string &address)
: _server(std::move(server))
, _client(address)
, _watcher(address, "pitaya", std::bind(&ServiceDiscovery::OnWatch, this, _1))
, _heartbeatTTL(60s)
, _leaseId(0)
{
//    auto response_task = _client.get("pitaya/servers/connector/48f99e38-ba01-4e35-a4f7-22023026d198");
    auto response_task = _client.ls("pitaya/servers");
    auto resp = response_task.get();
    if (resp.is_ok()) {
        for (size_t i = 0; i < resp.keys.size(); ++i) {
            cout << resp.keys[i] << " = " << resp.values[i].value << endl;
        }
//        cout << resp.value.value << std::endl;
    } else {
        cout << "Error getting key: " << resp.status.etcd_error_message << endl;
    }

    Configure();
}

void
service_discovery::ServiceDiscovery::AddListener(const Listener &listener)
{
    _listeners.push_back(listener);
}

void
service_discovery::ServiceDiscovery::Init()
{
    _running = true;
//    CreateLease();
}

void
service_discovery::ServiceDiscovery::OnWatch(etcd::Response res)
{
    cout << "OnWatch CALLED!" << endl;
}

int64_t
service_discovery::ServiceDiscovery::CreateLease()
{
    etcd::Response res = _client.leasegrant(_heartbeatTTL.count()).get();

    if (!res.is_ok()) {
        cerr << "leasegrant() failed!" << endl;
        return -1;
    }

    // TODO: Keep lease alive by renewing the lease every X seconds (do this in a thread or async channel)
    return res.value.lease_id;
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
    etcd::Response res = _client.set(_etcdPrefix + GetServerKey(server.id, server.type), ServerAsJson(server),  _leaseId).get();
    return res.status;
}

static bool
ParseEtcdKey(const string &key, string &serverType, string &serverId)
{
    auto comps = string_utils::Split(key, '/');
    if (comps.size() != 3) {
        cerr << "Error parsing etcd key " << key << "(server name can't contain /)" << endl;
        return false;
    }

    serverType = comps[1];
    serverId = comps[2];
    return true;
}

void
service_discovery::ServiceDiscovery::AddServer(std::shared_ptr<Server> server)
{
//    if _, loaded := sd.serverMapByID.LoadOrStore(sv.ID, sv); !loaded {
//        mapSvByType, ok := sd.serverMapByType.Load(sv.Type)
//        if !ok {
//            mapSvByType = make(map[string]*Server)
//            sd.serverMapByType.Store(sv.Type, mapSvByType)
//        }
//        mapSvByType.(map[string]*Server)[sv.ID] = sv
//        if sv.ID != sd.server.ID {
//            sd.notifyListeners(ADD, sv)
//        }
//    }
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    if (_serversById[server->id] == nullptr) {
        _serversById[server->id] = server;
        _serversByType[server->type][server->id] = server;
        if (server->id != _server->id) {
        }
    }
}

std::shared_ptr<pitaya::Server> service_discovery::ServiceDiscovery::GetServerByID(const string& id){
    std::lock_guard<decltype(_serversById)> lock(_serversById);
    return _serversById[id];
}

std::shared_ptr<pitaya::Server>
service_discovery::ServiceDiscovery::GetServerFromEtcd(const std::string &serverId, const std::string &serverType)
{
    auto serverKey = GetServerKey(serverId, serverType);

    try {
        etcd::Response res = _client.get(serverKey).get();

        if (!res.is_ok()) {
            cerr << "Failed to get key from server:" << "\n";
            if (!res.status.etcd_is_ok()) cerr << "Etcd: " << res.status.etcd_error_message << "\n";
            if (!res.status.grpc_is_ok()) cerr << "gRPC: " << res.status.grpc_error_message << "\n";
            return nullptr;
        }

        return ParseServer(res.value.value);
    } catch (const std::exception &exc) {
        cerr << "Error in communication with server: " << exc.what() << "\n";
        return nullptr;
    }
}

void
service_discovery::ServiceDiscovery::SyncServers()
{
    etcd::Response res = _client.ls("pitaya").get();
    if (!res.is_ok()) {
        cerr << "Error synchronizing servers" << endl;
        return;
    }

    vector<string> allIds;

    for (size_t i = 0; i < res.keys.size(); ++i) {
        // 1. Parse key
        string serverType, serverId;
        bool ok = ParseEtcdKey(res.keys[i], serverType, serverId);

        if (!ok) {
            cerr << "Failed to parse etcd key " << res.keys[i] << endl;
        }

        allIds.push_back(serverId);

        {
            std::lock_guard<decltype(_serversById)> lock(_serversById);
            if (_serversById[serverId] == nullptr) {
                cout << "Loading info from missing server: " << serverType << "/" << serverId << "\n";
                auto server = GetServerFromEtcd(serverId, serverType);
                if (!server) {
                    cerr << "Error getting server from etcd: " << serverId << "\n";
                    continue;
                }
                AddServer(std::move(server));
            }
        }

        string svType = res.keys[i];
        string svId = res.keys[i];

        cout << res.keys[i] << " = " << res.values[i].value << endl;

    }
}

void
service_discovery::ServiceDiscovery::Configure()
{
    _syncServersInterval = 2s;
    _heartbeatTTL = 1s;
    _logHeartbeat = true;
    _etcdPrefix = "pitaya";
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

static std::shared_ptr<pitaya::Server>
ParseServer(const string &jsonStr)
{
    auto server = std::make_shared<pitaya::Server>();

    json::value jsonSrv;
    jsonSrv.parse(jsonStr);

    if (!jsonSrv.is_object()) {
        cerr << "Server json is not an object" << "\n";
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
    // TODO: Add fmt library to improve this code.
    return std::string("servers") + std::string("/") + serverType + std::string("/") + serverId;
}
