#include "pitaya/etcd_client_v3.h"

#include "pitaya.h"

#include "spdlog/sinks/stdout_color_sinks.h"

using std::placeholders::_1;

namespace pitaya {

EtcdClientV3::EtcdClientV3(const std::string& endpoint,
                           const std::string& prefix,
                           bool logHeartbeat,
                           std::chrono::seconds grpcTimeout,
                           const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("etcd_client_v3")
                      : spdlog::stdout_color_mt("etcd_client_v3"))
    , _endpoint(endpoint)
    , _prefix(prefix)
    , _client(endpoint)
{
    _client.set_grpc_timeout(grpcTimeout);
}

LeaseGrantResponse
EtcdClientV3::LeaseGrant(std::chrono::seconds seconds)
{
    etcd::Response etcdRes = _client.leasegrant(seconds.count()).get();

    LeaseGrantResponse res;
    res.ok = etcdRes.is_ok();
    res.leaseId = etcdRes.value().lease();
    if (!res.ok) {
        res.errorMsg = etcdRes.error_message();
    }

    return res;
}

LeaseRevokeResponse
EtcdClientV3::LeaseRevoke(int64_t leaseId)
{
    etcd::Response etcdRes = _client.leaserevoke(leaseId).get();

    LeaseRevokeResponse res;
    res.ok = etcdRes.is_ok();
    if (!res.ok) {
        res.errorMsg = etcdRes.error_message();
    }
    return res;
}

SetResponse
EtcdClientV3::Set(const std::string& key, const std::string& val, int64_t leaseId)
{
    etcd::Response etcdRes = _client.set(key, val, leaseId).get();
    SetResponse res;
    res.ok = etcdRes.is_ok();
    if (!res.ok) {
        res.errorMsg = etcdRes.error_message();
    }
    return res;
}

GetResponse
EtcdClientV3::Get(const std::string& key)
{
    etcd::Response etcdRes = _client.get(key).get();

    GetResponse res;
    res.ok = etcdRes.is_ok();
    res.value = etcdRes.value().as_string();
    if (!res.ok) {
        res.errorMsg = etcdRes.error_message();
    }
    return res;
}

ListResponse
EtcdClientV3::List(const std::string& prefix)
{
    etcd::Response etcdRes = _client.ls(prefix).get();

    ListResponse res;
    res.ok = etcdRes.is_ok();
    res.keys = std::move(etcdRes.keys());
    if (!res.ok) {
        res.errorMsg =  etcdRes.error_message();
    }
    return res;
}

void
EtcdClientV3::LeaseKeepAlive(int64_t ttl, int64_t _leaseId, std::function<void(std::exception_ptr)> onExit)
{
    etcd::KeepAlive _leaseKeepAlive(_client, onExit, ttl, leaseId);
}

void
EtcdClientV3::StopLeaseKeepAlive()
{
    std::cout << "Stopping lease keepalive" << std::endl;
     _leaseKeepAlive.reset();
     std::cout << "Stopped lease keepalive" << std::endl;
}

void
EtcdClientV3::Watch(std::function<void(WatchResponse)> onWatch)
{
    try {
        _onWatch = std::move(onWatch);
        _watcher = std::unique_ptr<etcd::Watcher>(
            new etcd::Watcher(_endpoint, _prefix, std::bind(&EtcdClientV3::OnWatch, this, _1)));
    } catch (const std::runtime_error& exc) {
        throw PitayaException(exc.what());
    }
}

void
EtcdClientV3::CancelWatch()
{
    _watcher.reset();
}

void
EtcdClientV3::OnWatch(etcd::Response res)
{
    WatchResponse watchRes = {};
    watchRes.ok = res.is_ok();
    watchRes.action = res.action();
    watchRes.key = res.value().key();
    watchRes.value = res.value().as_string();
    _onWatch(watchRes);
}

} // namespace pitaya
