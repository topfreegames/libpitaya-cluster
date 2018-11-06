#ifndef SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H
#define SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H

#include "spdlog/spdlog.h"
#include <etcd/Client.hpp>
#include <functional>
#include <future>
#include <pplx/pplxtasks.h>
#include <string>

namespace pitaya {
namespace service_discovery {

enum class LeaseKeepAliveStatus
{
    Ok,
    Fail
};

class LeaseKeepAlive
{
public:
    LeaseKeepAlive(etcd::Client& client, const char* loggerName);
    pplx::task<LeaseKeepAliveStatus> Start();
    void Stop();
    void SetLeaseId(int64_t leaseId);

private:
    LeaseKeepAliveStatus TickWrapper();

private:
    std::shared_ptr<spdlog::logger> _log;
    etcd::Client& _client;
    int64_t _leaseId;
    std::promise<void> _donePromise;
    std::shared_future<void> _doneFuture;
    pplx::task<LeaseKeepAliveStatus> _runTask;
};

} // namespace service_discovery
} // namespace pitaya

#endif // SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H
