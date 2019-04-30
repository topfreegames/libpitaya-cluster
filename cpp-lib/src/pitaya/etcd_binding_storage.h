#ifndef PITAYA_ETCD_BINDING_STORAGE_H
#define PITAYA_ETCD_BINDING_STORAGE_H

#include "pitaya/etcd_config.h"
#include "pitaya/binding_storage.h"
#include "pitaya/etcd_client.h"

#include "spdlog/logger.h"

#include <chrono>

namespace pitaya {

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
};

} // namespace pitaya

#endif // PITAYA_ETCD_BINDING_STORAGE_H
