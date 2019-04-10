#ifndef PITAYA_ETCD_CLIENT_V3_H
#define PITAYA_ETCD_CLIENT_V3_H

#include "pitaya/etcd_client.h"
#include "pitaya/etcd_lease_keep_alive.h"

#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>

namespace pitaya {

class EtcdClientV3 : public EtcdClient
{
public:
    EtcdClientV3(const std::string& endpoints,
                 const std::string& prefix,
                 bool logHeartbeat,
                 const char* loggerName = nullptr);

    LeaseGrantResponse LeaseGrant(std::chrono::seconds seconds) override;
    LeaseRevokeResponse LeaseRevoke(int64_t leaseId) override;
    SetResponse Set(const std::string& key, const std::string& val, int64_t leaseId) override;
    GetResponse Get(const std::string& key) override;
    ListResponse List(const std::string& prefix) override;
    void Watch(std::function<void(WatchResponse)> onWatch) override;
    void CancelWatch() override;

    void LeaseKeepAlive(int64_t leaseId,
                        std::function<void(EtcdLeaseKeepAliveStatus)> onExit) override;
    void StopLeaseKeepAlive() override;

private:
    void OnWatch(etcd::Response res);

private:
    std::shared_ptr<spdlog::logger> _log;
    std::string _endpoint;
    std::string _prefix;
    etcd::Client _client;
    std::unique_ptr<etcd::Watcher> _watcher;
    std::function<void(WatchResponse)> _onWatch;
    EtcdLeaseKeepAlive _leaseKeepAlive;
};

} // namespace pitaya

#endif // PITAYA_ETCD_CLIENT_V3_H
