#ifndef PITAYA_ETCD_CONFIG_H
#define PITAYA_ETCD_CONFIG_H

#include <chrono>
#include <string>
#include <vector>

namespace pitaya {

struct EtcdServiceDiscoveryConfig
{
    std::string endpoints;
    std::string etcdPrefix;
    std::chrono::seconds heartbeatTTLSec;
    bool logHeartbeat;
    bool logServerSync;
    bool logServerDetails;
    std::chrono::seconds syncServersIntervalSec;
    int32_t maxNumberOfRetries;
    // List of server types to filter. This filter will be added on top
    // of the etcdPrefix.
    std::vector<std::string> serverTypeFilters;
    // Delay used on the exponential backoff retry (given in
    // milliseconds).
    int32_t retryDelayMilliseconds;

    EtcdServiceDiscoveryConfig()
        : heartbeatTTLSec(std::chrono::seconds(60))
        , logHeartbeat(true)
        , logServerSync(true)
        , logServerDetails(false)
        , syncServersIntervalSec(std::chrono::seconds(60))
        , maxNumberOfRetries(10)
        , retryDelayMilliseconds(100)
    {}
};

struct EtcdBindingStorageConfig
{
    std::chrono::seconds leaseTtl;
    std::string endpoint;
    std::string etcdPrefix;
};

} // namespace pitaya

#endif // PITAYA_ETCD_CONFIG_H
