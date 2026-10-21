#include "pitaya/etcd_client_v3.h"

#include "pitaya.h"
#include "pitaya/utils.h"

#include <future>
#include <thread>

using std::placeholders::_1;

namespace pitaya {

static constexpr std::chrono::seconds kStopLeaseKeepAliveTimeout(3);

EtcdClientV3::EtcdClientV3(const std::string& endpoint,
                           const std::string& prefix,
                           bool logHeartbeat,
                           std::chrono::seconds grpcTimeout,
                           const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName,"etcd_client_v3"))
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

LeaseTimeToLiveResponse
EtcdClientV3::LeaseTimeToLive(int64_t leaseId)
{
    etcd::Response etcdRes = _client.leasetimetolive(leaseId).get();

    LeaseTimeToLiveResponse res;
    res.ok = etcdRes.is_ok();
    res.ttl = etcdRes.value().ttl();
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
EtcdClientV3::LeaseKeepAlive(int64_t ttl, int64_t leaseId, std::function<void(std::exception_ptr)> onExit)
{

    _log->info("Starting keepalive with {} seconds interval for leaseID: {}", ttl/3, leaseId );

    auto gate = std::make_shared<LeaseKeepAliveHandlerGate>();
    auto handler = [gate, onExit](std::exception_ptr exc) {
        std::lock_guard<std::mutex> lock(gate->mutex);
        if (gate->open) {
            onExit(exc);
        }
    };
    auto keepAlive = std::make_shared<etcd::KeepAlive>(_client, handler, ttl/3, leaseId);

    std::lock_guard<std::mutex> lock(_leaseKeepAliveMutex);
    _leaseKeepAlive = std::move(keepAlive);
    _leaseKeepAliveHandlerGate = std::move(gate);
}

void
EtcdClientV3::StopLeaseKeepAlive()
{
    std::shared_ptr<etcd::KeepAlive> keepAlive;
    std::shared_ptr<LeaseKeepAliveHandlerGate> gate;
    {
        std::lock_guard<std::mutex> lock(_leaseKeepAliveMutex);
        keepAlive = std::move(_leaseKeepAlive);
        gate = std::move(_leaseKeepAliveHandlerGate);
    }

    if (gate) {
        std::lock_guard<std::mutex> lock(gate->mutex);
        gate->open = false;
    }

    if (!keepAlive) {
        return;
    }

    std::cout << "Stopping lease keepalive" << std::endl;

    auto stopped = std::make_shared<std::promise<void>>();
    auto stoppedFuture = stopped->get_future();
    std::thread([keepAlive = std::move(keepAlive), stopped]() mutable {
        keepAlive.reset();
        stopped->set_value();
    }).detach();

    if (stoppedFuture.wait_for(kStopLeaseKeepAliveTimeout) == std::future_status::timeout) {
        _log->warn("Lease keepalive did not stop in {} seconds, leaving it behind",
                   kStopLeaseKeepAliveTimeout.count());
        return;
    }
    std::cout << "Stopped lease keepalive" << std::endl;
}

void
EtcdClientV3::Watch(std::function<void(WatchResponse)> onWatch)
{
    try {
        _log->info("Starting ETCD Watcher with {} prefix.", _prefix );
        _onWatch = std::move(onWatch);
        _watcher = std::make_shared<etcd::Watcher>(_client, _prefix, std::bind(&EtcdClientV3::OnWatch, this, _1), true);

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
