#ifndef TFG_SERVICE_DISCOVERY_H
#define TFG_SERVICE_DISCOVERY_H

#include <memory>
#include <string>

#include "pitaya.h"
#include "pitaya/service_discovery/worker.h"
#include "spdlog/spdlog.h"
#include "pitaya/service_discovery/config.h"
#include <boost/optional.hpp>

namespace pitaya {
namespace service_discovery {

class ServiceDiscovery
{
public:
    ServiceDiscovery(const Config& config,
                     const pitaya::Server& server,
                     const char* loggerName = nullptr);
    ~ServiceDiscovery();

    boost::optional<pitaya::Server> GetServerById(const std::string& id);
    std::vector<pitaya::Server> GetServersByType(const std::string& type);

private:
    std::shared_ptr<spdlog::logger> _log;
    Worker _worker;
};

}
}

#endif // TFG_SERVICE_DISCOVERY_H
