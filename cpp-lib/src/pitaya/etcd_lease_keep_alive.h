#ifndef PITAYA_ETCD_LEASE_KEEP_ALIVE_H
#define PITAYA_ETCD_LEASE_KEEP_ALIVE_H

#include "pitaya/etcd_client.h"
#include "spdlog/spdlog.h"

#include <etcd/Client.hpp>
#include <functional>
#include <future>
#include <pplx/pplxtasks.h>
#include <string>

namespace pitaya {

class EtcdLeaseKeepAlive
{
public:
    EtcdLeaseKeepAlive(etcd::Client& client, bool shouldLog, const char* loggerName, int leaseTTL);
    pplx::task<EtcdLeaseKeepAliveStatus> Start();
    void Stop();
    void SetLeaseId(int64_t leaseId);

private:
    EtcdLeaseKeepAliveStatus TickWrapper();

private:
    bool _running;
    std::shared_ptr<spdlog::logger> _log;
    bool _shouldLog;
    etcd::Client& _client;
    int64_t _leaseId;
    int _leaseTTL;
    std::promise<void> _donePromise;
    std::shared_future<void> _doneFuture;
    pplx::task<EtcdLeaseKeepAliveStatus> _runTask;
};

} // namespace pitaya

#endif // SERVICE_DISCOVERY_LEASE_KEEP_ALIVE_H
