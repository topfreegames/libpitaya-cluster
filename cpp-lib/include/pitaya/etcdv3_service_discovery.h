#ifndef PITAYA_ETCDV3_SERVICE_DISCOVERY_H
#define PITAYA_ETCDV3_SERVICE_DISCOVERY_H

#include "pitaya.h"
#include "pitaya/etcd_client.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/service_discovery.h"
#include "spdlog/spdlog.h"

#include <boost/optional.hpp>
#include <memory>
#include <string>

namespace pitaya {
namespace etcdv3_service_discovery {

class Worker;

class Etcdv3ServiceDiscovery : public service_discovery::ServiceDiscovery
{
public:
    Etcdv3ServiceDiscovery(const Config& config,
                           const pitaya::Server& server,
                           std::unique_ptr<EtcdClient> etcdClient,
                           const char* loggerName = nullptr);
    ~Etcdv3ServiceDiscovery();

    boost::optional<pitaya::Server> GetServerById(const std::string& id) override;
    std::vector<pitaya::Server> GetServersByType(const std::string& type) override;
    void AddListener(service_discovery::Listener* listener) override;

private:
    std::shared_ptr<spdlog::logger> _log;
    std::unique_ptr<Worker> _worker;
};

} // namespace etcdv3_service_discovery
} // namespace pitaya

#endif // PITAYA_ETCDV3_SERVICE_DISCOVERY_H
