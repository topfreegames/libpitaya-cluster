#include "pitaya/etcd_client.h"

#include "pitaya.h"

#include "spdlog/sinks/stdout_color_sinks.h"

using std::placeholders::_1;

namespace pitaya {

EtcdClientV3::EtcdClientV3(const std::string& endpoint,
                           const std::string& prefix,
                           bool logHeartbeat,
                           const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("etcd_client_v3")
                      : spdlog::stdout_color_mt("etcd_client_v3"))
    , _endpoint(endpoint)
    , _prefix(prefix)
    , _client(endpoint)
    , _leaseKeepAlive(_client, logHeartbeat, loggerName)
{}

LeaseGrantResponse
EtcdClientV3::LeaseGrant(std::chrono::seconds seconds)
{
    etcd::Response etcdRes = _client.leasegrant(seconds.count()).get();

    LeaseGrantResponse res;
    res.ok = etcdRes.is_ok();
    res.leaseId = etcdRes.value.lease_id;
    if (!res.ok) {
        res.errorMsg = (etcdRes.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR)
                           ? "gRPC error: " + etcdRes.status.grpc_error_message
                           : "etcd error: " + etcdRes.status.etcd_error_message;
    }

    return res;
}

LeaseRevokeResponse
EtcdClientV3::LeaseRevoke(int64_t leaseId)
{
    etcd::Response etcdRes = _client.lease_revoke(leaseId).get();

    LeaseRevokeResponse res;
    res.ok = etcdRes.is_ok();
    if (!res.ok) {
        res.errorMsg = (etcdRes.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR)
                           ? "gRPC error: " + etcdRes.status.grpc_error_message
                           : "etcd error: " + etcdRes.status.etcd_error_message;
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
        res.errorMsg = (etcdRes.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR)
                           ? "gRPC error: " + etcdRes.status.grpc_error_message
                           : "etcd error: " + etcdRes.status.etcd_error_message;
    }
    return res;
}

GetResponse
EtcdClientV3::Get(const std::string& key)
{
    etcd::Response etcdRes = _client.get(key).get();

    GetResponse res;
    res.ok = etcdRes.is_ok();
    res.value = etcdRes.value.value;
    if (!res.ok) {
        res.errorMsg = (etcdRes.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR)
                           ? "gRPC error: " + etcdRes.status.grpc_error_message
                           : "etcd error: " + etcdRes.status.etcd_error_message;
    }
    return res;
}

ListResponse
EtcdClientV3::List(const std::string& prefix)
{
    etcd::Response etcdRes = _client.ls(prefix).get();

    ListResponse res;
    res.ok = etcdRes.is_ok();
    res.keys = std::move(etcdRes.keys);
    if (!res.ok) {
        res.errorMsg = (etcdRes.status.etcd_error_code == etcd::StatusCode::UNDERLYING_GRPC_ERROR)
                           ? "gRPC error: " + etcdRes.status.grpc_error_message
                           : "etcd error: " + etcdRes.status.etcd_error_message;
    }
    return res;
}

void
EtcdClientV3::LeaseKeepAlive(int64_t leaseId, std::function<void(EtcdLeaseKeepAliveStatus)> onExit)
{
    _leaseKeepAlive.SetLeaseId(leaseId);
    _leaseKeepAlive.Start().then(std::move(onExit));
}

void
EtcdClientV3::StopLeaseKeepAlive()
{
    _leaseKeepAlive.Stop();
}

void
EtcdClientV3::Watch(std::function<void(WatchResponse)> onWatch)
{
    try {
        _onWatch = std::move(onWatch);
        _watcher = std::unique_ptr<etcd::Watcher>(
            new etcd::Watcher(_endpoint, _prefix, std::bind(&EtcdClientV3::OnWatch, this, _1)));
    } catch (const etcd::watch_error& exc) {
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
    watchRes.action = res.action;
    watchRes.key = res.value.key;
    watchRes.value = res.value.value;
    _onWatch(watchRes);
}

} // namespace pitaya
