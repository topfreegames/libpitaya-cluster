#include "pitaya/etcd_binding_storage.h"

#include "pitaya.h"
#include "pitaya/utils.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

static std::string GetUserBindingKey(const std::string& prefix,
                                     const std::string& uid,
                                     const std::string& frontendType);

namespace pitaya {

static constexpr const char* kLogTag = "etcd_binding_storage";

EtcdBindingStorage::EtcdBindingStorage(EtcdBindingStorageConfig config,
                                       std::unique_ptr<EtcdClient> etcdClient,
                                       const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _config(std::move(config))
    , _etcdClient(std::move(etcdClient))
{}

EtcdBindingStorage::~EtcdBindingStorage()
{}

std::string
EtcdBindingStorage::GetUserFrontendId(const std::string& uid, const std::string& frontendType)
{
    auto res = _etcdClient->Get(GetUserBindingKey(_config.etcdPrefix, uid, frontendType));
    if (!res.ok) {
        throw PitayaException(fmt::format("Failed to get user frontend id: {}", res.errorMsg));
    }
    return res.value;
}

} // namespace pitaya

//
// Helper functions
//

static std::string
GetUserBindingKey(const std::string& prefix,
                  const std::string& uid,
                  const std::string& frontendType)
{
    return fmt::format("{}bindings/{}/{}", prefix, frontendType, uid);
}
