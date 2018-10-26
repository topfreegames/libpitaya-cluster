#include "service_discovery.h"
#include <nats/nats.h>
#include <cpprest/json.h>
#include "string_utils.h"

using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::cout;
using std::cerr;
using std::endl;
using std::chrono::seconds;
namespace json = web::json;

using std::placeholders::_1;
using namespace std::chrono_literals;

static string ServerAsJson(const service_discovery::Server &server);

service_discovery::ServiceDiscovery::ServiceDiscovery(unique_ptr<Server> server, const string &address)
: _server(std::move(server))
, _client(address)
, _watcher(address, "pitaya", std::bind(&ServiceDiscovery::OnWatch, this, _1))
, _leaseId(0)
, _heartbeatTTL(60s)
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
    etcd::Response res = _client.set(_etcdPrefix + server.GetKey(), ServerAsJson(server),  _leaseId).get();
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
service_discovery::ServiceDiscovery::SyncServers()
{
    etcd::Response res = _client.ls("pitaya").get();
    if (!res.is_ok()) {
        cerr << "Error synchronizing servers" << endl;
        return;
    }

    for (size_t i = 0; i < res.keys.size(); ++i) {
        // 1. Parse key
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
ServerAsJson(const service_discovery::Server &server)
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

