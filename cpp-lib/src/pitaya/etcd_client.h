#ifndef PITAYA_ETCD_CLIENT_H
#define PITAYA_ETCD_CLIENT_H

#include "spdlog/spdlog.h"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace pitaya {

struct WatchResponse
{
    bool ok;
    std::string action;
    std::string key;
    std::string value;
};

struct LeaseGrantResponse
{
    bool ok;
    std::string errorMsg;
    int64_t leaseId;
};

struct LeaseRevokeResponse
{
    bool ok;
    std::string errorMsg;
};

struct SetResponse
{
    bool ok;
    std::string errorMsg;
};

struct ListResponse
{
    bool ok;
    std::string errorMsg;
    std::vector<std::string> keys;
};

struct GetResponse
{
    bool ok;
    std::string errorMsg;
    std::string value;
};

enum class EtcdLeaseKeepAliveStatus
{
    Ok,
    Fail,
};

class EtcdClient
{
public:
    virtual ~EtcdClient() = default;

    virtual LeaseGrantResponse LeaseGrant(std::chrono::seconds seconds) = 0;
    virtual LeaseRevokeResponse LeaseRevoke(int64_t leaseId) = 0;
    virtual SetResponse Set(const std::string& key, const std::string& val, int64_t leaseId) = 0;
    virtual GetResponse Get(const std::string& key) = 0;
    virtual ListResponse List(const std::string& prefix) = 0;
    virtual void Watch(std::function<void(WatchResponse)> onWatch) = 0;
    virtual void CancelWatch() = 0;

    virtual void LeaseKeepAlive(int64_t leaseId,
                                std::function<void(EtcdLeaseKeepAliveStatus)> onExit) = 0;
    virtual void StopLeaseKeepAlive() = 0;
};

} // namespace pitaya

#endif // PITAYA_ETCD_CLIENT_H
