#ifndef PITAYA_ETCDV3_SERVICE_DISCOVERY_H
#define PITAYA_ETCDV3_SERVICE_DISCOVERY_H

#include <memory>
#include <string>

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/etcdv3_service_discovery/worker.h"
#include "pitaya/service_discovery.h"
#include "spdlog/spdlog.h"
#include <boost/optional.hpp>

namespace pitaya {
namespace etcdv3_service_discovery {

class Etcdv3ServiceDiscovery : public service_discovery::ServiceDiscovery
{
public:
    Etcdv3ServiceDiscovery(const Config& config,
                           const pitaya::Server& server,
                           const char* loggerName = nullptr);
    ~Etcdv3ServiceDiscovery();

    boost::optional<pitaya::Server> GetServerById(const std::string& id) override;
    std::vector<pitaya::Server> GetServersByType(const std::string& type) override;

private:
    std::shared_ptr<spdlog::logger> _log;
    Worker _worker;
};

} // namespace etcdv3_service_discovery
} // namespace pitaya

#endif // PITAYA_ETCDV3_SERVICE_DISCOVERY_H
