#include "pitaya/etcd_binding_storage.h"

#include "pitaya.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

static std::string GetUserBindingKey(const std::string& uid, const std::string& frontendType);

namespace pitaya {

static constexpr const char* kLogTag = "etcd_binding_storage";

EtcdBindingStorage::EtcdBindingStorage(EtcdBindingStorageConfig config,
                                       std::unique_ptr<EtcdClient> etcdClient,
                                       const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _config(std::move(config))
    , _etcdClient(std::move(etcdClient))
    , _leaseId(-1)
{
    using std::placeholders::_1;

    auto res = _etcdClient->LeaseGrant(_config.leaseTtl);
    if (!res.ok) {
        throw PitayaException(fmt::format("Lease grant failed: {}", res.errorMsg));
    }

    _leaseId = res.leaseId;
    _etcdClient->LeaseKeepAlive(res.leaseId, [this](EtcdLeaseKeepAliveStatus status) {
        switch (status) {
            case EtcdLeaseKeepAliveStatus::Ok: {
                _log->debug("lease keep alive finished");
                break;
            }
            case EtcdLeaseKeepAliveStatus::Fail: {
                _log->error("Lease keep alive failed");
                break;
            }
        }
    });
}

EtcdBindingStorage::~EtcdBindingStorage()
{
    _log->debug("Stopping lease keep alive");
    _etcdClient->StopLeaseKeepAlive();
    spdlog::drop(kLogTag);
}

std::string
EtcdBindingStorage::GetUserFrontendId(const std::string& uid, const std::string& frontendType)
{
    auto res = _etcdClient->Get(GetUserBindingKey(uid, frontendType));
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
GetUserBindingKey(const std::string& uid, const std::string& frontendType)
{
    return fmt::format("bindings/{}/{}", frontendType, uid);
}
