#include "lease_keep_alive.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <chrono>

using std::chrono::seconds;

service_discovery::LeaseKeepAlive::LeaseKeepAlive(etcd::Client &client)
: _log(spdlog::get("service_discovery")->clone("lease_keep_alive"))
, _client(client)
, _leaseId(-1)
, _donePromise()
, _doneFuture(_donePromise.get_future())
{
    _log->set_level(spdlog::level::debug);
}

void
service_discovery::LeaseKeepAlive::Start()
{
    if (_leaseId == -1) {
        _log->error("lease id is -1, not starting.");
        return;
    }

    _log->info("Starting LeaseKeepAlive");
    _runFuture = std::async(std::launch::async, &LeaseKeepAlive::TickWrapper, this);
}

void
service_discovery::LeaseKeepAlive::Stop()
{
    _log->info("Stopping");
    _donePromise.set_value();
}

void
service_discovery::LeaseKeepAlive::TickWrapper()
{
    _log->info("Thread started");

    auto status = std::future_status::timeout;
    while (status != std::future_status::ready) {
        _log->debug("Renewing lease");
        auto res = _client.lease_keep_alive(_leaseId).get();
        if (!res.is_ok()) {
            _log->error("Failed to renew lease, stopping");
            break;
        }

        if (res.value.ttl < 1) {
            _log->info("Received TTL of {} seconds, stopping", res.value.ttl);
            break;
        }

        auto waitFor = seconds{static_cast<int>(res.value.ttl/3.0f)};

        _log->debug("Lease renewed for {} seconds, waiting {} for renew", res.value.ttl, waitFor.count());

        status = _doneFuture.wait_for(waitFor);
    }
    _log->info("Stopped.");
}

void
service_discovery::LeaseKeepAlive::SetLeaseId(int64_t leaseId)
{
    _leaseId = leaseId;
}
