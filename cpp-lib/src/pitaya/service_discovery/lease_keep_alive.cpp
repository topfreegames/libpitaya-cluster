#include "pitaya/service_discovery/lease_keep_alive.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <chrono>

using std::chrono::seconds;

namespace pitaya {
namespace service_discovery {

LeaseKeepAlive::LeaseKeepAlive(etcd::Client& client)
    : _log(spdlog::get("service_discovery")->clone("lease_keep_alive"))
    , _client(client)
    , _leaseId(-1)
    , _donePromise()
    , _doneFuture(_donePromise.get_future())
{
    _log->set_level(spdlog::level::debug);
}

pplx::task<service_discovery::LeaseKeepAliveStatus>
LeaseKeepAlive::Start()
{
    if (_leaseId == -1) {
        _log->error("lease id is -1, not starting.");
        return pplx::task_from_result(LeaseKeepAliveStatus::Fail);
    }

    _donePromise = std::promise<void>();
    _doneFuture = _donePromise.get_future();

    _log->info("Starting LeaseKeepAlive");
    _runTask = pplx::create_task(std::bind(&LeaseKeepAlive::TickWrapper, this));
    return _runTask;
}

void
LeaseKeepAlive::Stop()
{
    _log->info("Stopping");
    _donePromise.set_value();
    _runTask.wait();
}

LeaseKeepAliveStatus
LeaseKeepAlive::TickWrapper()
{
    _log->info("Thread started: lease id: {}", _leaseId);

    auto status = std::future_status::timeout;
    while (status != std::future_status::ready) {
        _log->debug("Renewing lease");

        auto res = _client.lease_keep_alive(_leaseId).get();
        if (!res.is_ok()) {
            _log->error("Failed to renew lease, stopping");
            _leaseId = -1;
            return LeaseKeepAliveStatus::Fail;
        }

        if (res.value.ttl < 1) {
            _log->info("Received TTL of {} seconds, stopping", res.value.ttl);
            _leaseId = -1;
            return LeaseKeepAliveStatus::Fail;
        }

        auto waitFor = seconds{ static_cast<int>(res.value.ttl / 3.0f) };

        _log->debug("Lease {} renewed for {} seconds, waiting {} for renew",
                    _leaseId,
                    res.value.ttl,
                    waitFor.count());

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

}
}
