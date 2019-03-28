#ifndef PITAYA_ETCD_BINDING_STORAGE_H
#define PITAYA_ETCD_BINDING_STORAGE_H

#include "pitaya/binding_storage.h"
#include "pitaya/etcd_client.h"

#include "spdlog/logger.h"

#include <chrono>

namespace pitaya {

struct EtcdBindingStorageConfig
{
    std::chrono::seconds leaseTtl;
    std::string endpoint;
    std::string etcdPrefix;
};

class EtcdBindingStorage : public BindingStorage
{
public:
    EtcdBindingStorage(EtcdBindingStorageConfig config,
                       std::unique_ptr<EtcdClient> etcdClient,
                       const char* loggerName = nullptr);
    ~EtcdBindingStorage();

    std::string GetUserFrontendId(const std::string& uid, const std::string& frontendType) override;

private:
    // void OnWatch(WatchResponse res);

private:
    std::shared_ptr<spdlog::logger> _log;
    EtcdBindingStorageConfig _config;
    std::unique_ptr<EtcdClient> _etcdClient;
    int64_t _leaseId;
};

} // namespace pitaya

#endif // PITAYA_ETCD_BINDING_STORAGE_H