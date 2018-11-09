#ifndef PITAYA_SERVICE_DISCOVERY_H
#define PITAYA_SERVICE_DISCOVERY_H

#include <memory>
#include <string>

#include "pitaya.h"
#include <boost/optional.hpp>

namespace pitaya {
namespace service_discovery {

class ServiceDiscovery
{
public:
    virtual ~ServiceDiscovery() = default;

    virtual boost::optional<pitaya::Server> GetServerById(const std::string& id) = 0;
    virtual std::vector<pitaya::Server> GetServersByType(const std::string& type) = 0;
};

} // namespace service_discovery
} // namespace pitaya

#endif // PITAYA_SERVICE_DISCOVERY_H
