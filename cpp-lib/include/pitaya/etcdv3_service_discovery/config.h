#ifndef PITAYA_ETCDV3_SERVICE_DISCOVERY_CONFIG_H
#define PITAYA_ETCDV3_SERVICE_DISCOVERY_CONFIG_H

#include <chrono>
#include <string>

namespace pitaya {
namespace etcdv3_service_discovery {

struct Config
{
    std::string endpoints;
    std::string etcdPrefix;
    std::chrono::seconds heartbeatTTLSec;
    bool logHeartbeat;
    bool logServerSync;
    bool logServerDetails;
    std::chrono::seconds syncServersIntervalSec;

    Config()
        : heartbeatTTLSec(std::chrono::seconds(60))
        , logHeartbeat(true)
        , logServerSync(true)
        , logServerDetails(false)
        , syncServersIntervalSec(std::chrono::seconds(60))
    {}
};

} // namespace etcdv3_service_discovery
} // namespace pitaya

#endif // PITAYA_ETCDV3_SERVICE_DISCOVERY_CONFIG_H
