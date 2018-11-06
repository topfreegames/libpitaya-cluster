#ifndef PITAYA_SERVICE_DISCOVERY_CONFIG_H
#define PITAYA_SERVICE_DISCOVERY_CONFIG_H

#include <string>
#include <chrono>

namespace pitaya {
namespace service_discovery {

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

}
}

#endif // PITAYA_SERVICE_DISCOVERY_CONFIG_H
