#ifndef SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H
#define SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H

#include <string>
#include <future>
#include <functional>
#include <etcd/Client.hpp>
#include "spdlog/spdlog.h"

namespace service_discovery {

    class LeaseKeepAlive {
    public:
        LeaseKeepAlive(etcd::Client &client);
        void Start();
        void Stop();
        void SetLeaseId(int64_t leaseId);

    private:
        void TickWrapper();
        void TickFunction();

    private:
        std::shared_ptr<spdlog::logger> _log;
        etcd::Client &_client;
        int64_t _leaseId;
        std::promise<void> _donePromise;
        std::shared_future<void> _doneFuture;
        std::future<void> _runFuture;
    };

} // namespace service_discovery

#endif // SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H
