#include "pitaya/etcdv3_service_discovery/lease_keep_alive.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <chrono>

using std::chrono::seconds;

namespace pitaya {
namespace etcdv3_service_discovery {

LeaseKeepAlive::LeaseKeepAlive(etcd::Client& client, bool shouldLog, const char* loggerName)
    : _running(false)
    , _log(spdlog::get(loggerName)->clone("lease_keep_alive"))
    , _shouldLog(shouldLog)
    , _client(client)
    , _leaseId(-1)
    , _donePromise()
    , _doneFuture(_donePromise.get_future())
{}

pplx::task<etcdv3_service_discovery::LeaseKeepAliveStatus>
LeaseKeepAlive::Start()
{
    if (_leaseId == -1) {
        _log->error("lease id is -1, not starting.");
        return pplx::task_from_result(LeaseKeepAliveStatus::Fail);
    }

    _running = true;
    _donePromise = std::promise<void>();
    _doneFuture = _donePromise.get_future();

    _log->info("Starting LeaseKeepAlive");
    _runTask = pplx::create_task(std::bind(&LeaseKeepAlive::TickWrapper, this));
    return _runTask;
}

void
LeaseKeepAlive::Stop()
{
    if (_running) {
        _running = false;
        _log->info("Stopping");
        _donePromise.set_value();
        _runTask.wait();
    }
}

LeaseKeepAliveStatus
LeaseKeepAlive::TickWrapper()
{
    _log->info("Thread started: lease id: {}", _leaseId);

    auto status = std::future_status::timeout;
    while (status != std::future_status::ready) {
        if (_shouldLog)
            _log->debug("Renewing lease");

        auto res = _client.lease_keep_alive(_leaseId).get();
        if (!res.is_ok()) {
            if (res.status.etcd_error_code == etcdv3::V3StatusCode::UNDERLYING_GRPC_ERROR) {
                _log->error("Failed to renew lease, stopping ({})", res.status.grpc_error_message);
            } else {
                _log->error("Failed to renew lease, stopping ({})", res.status.etcd_error_message);
            }
            _leaseId = -1;
            return LeaseKeepAliveStatus::Fail;
        }

        if (res.value.ttl < 1) {
            if (_shouldLog)
                _log->warn("Received TTL of {} seconds, stopping", res.value.ttl);
            _leaseId = -1;
            return LeaseKeepAliveStatus::Fail;
        }

        auto waitFor = seconds{ static_cast<int>(res.value.ttl / 3.0f) };

        if (_shouldLog) {
            _log->debug("Lease {} renewed for {} seconds, waiting {} for renew",
                        _leaseId,
                        res.value.ttl,
                        waitFor.count());
        }

        status = _doneFuture.wait_for(waitFor);
    }
    _log->info("Stopped.");
    return LeaseKeepAliveStatus::Ok;
}

void
LeaseKeepAlive::SetLeaseId(int64_t leaseId)
{
    _leaseId = leaseId;
}

} // namespace etcdv3_service_discovery
} // namespace pitaya
