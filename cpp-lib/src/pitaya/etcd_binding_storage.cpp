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

    // _etcdClient->Watch(std::bind(&EtcdBindingStorage::OnWatch, this, _1));
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

// void
// EtcdBindingStorage::OnWatch(WatchResponse res)
// {
//     if (!res.ok) {
//         _log->error("OnWatch error");
//         return;
//     }

//     // First we need to parse the etcd key to figure out if it
//     // belongs to the same prefix and it is actually a server.
//     string serverType, serverId;
//     if (!utils::ParseEtcdKey(res.key, _config.etcdPrefix, serverType, serverId)) {
//         _log->debug("OnWatch: Ignoring {}", res.key);
//         return;
//     }

//     if (res.action == "create") {
//         auto server = ParseServer(res.value);
//         if (!server) {
//             _log->error("OnWatch: Error parsing server: {}", res.value);
//             return;
//         }
//         AddServer(std::move(server.value()));
//         PrintServers();
//     } else if (res.action == "delete") {
//         DeleteServer(serverId);
//         PrintServers();
//     }
// }

} // namespace pitaya

//
// Helper functions
//

static std::string
GetUserBindingKey(const std::string& uid, const std::string& frontendType)
{
    return fmt::format("bindings/{}/{}", frontendType, uid);
}
