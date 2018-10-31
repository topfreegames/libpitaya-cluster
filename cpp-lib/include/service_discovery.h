#ifndef TFG_SERVICE_DISCOVERY_H
#define TFG_SERVICE_DISCOVERY_H

#include <string>
#include <memory>
#include <chrono>
#include <functional>

#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include <boost/optional.hpp>
#include "sync_map.h"
#include "pitaya.h"
#include "lease_keep_alive.h"
#include "spdlog/spdlog.h"
#include "service_discovery_worker.h"

namespace service_discovery {

    class ServiceDiscovery {
    public:
        ServiceDiscovery(pitaya::Server server, const std::string &address);
        ~ServiceDiscovery();

        boost::optional<pitaya::Server> GetServerById(const std::string &id);
        std::vector<pitaya::Server> GetServersByType(const std::string &type);

    private:
        std::shared_ptr<spdlog::logger> _log;
        ServiceDiscoveryWorker _worker;
    };

} // namespace service_discovery

#endif // TFG_SERVICE_DISCOVERY_H
