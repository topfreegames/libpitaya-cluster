#ifndef PITAYA_SERVICE_DISCOVERY_H
#define PITAYA_SERVICE_DISCOVERY_H

#include "pitaya.h"

#include <boost/optional.hpp>
#include <memory>
#include <string>

namespace pitaya {
namespace service_discovery {

class Listener
{
public:
    virtual ~Listener() = default;

    virtual void ServerAdded(const pitaya::Server& server) = 0;
    virtual void ServerRemoved(const pitaya::Server& server) = 0;
};

class ServiceDiscovery
{
public:
    virtual ~ServiceDiscovery() = default;

    virtual boost::optional<pitaya::Server> GetServerById(const std::string& id) = 0;
    virtual std::vector<pitaya::Server> GetServersByType(const std::string& type) = 0;
    virtual void AddListener(Listener* listener) = 0;
};

} // namespace service_discovery
} // namespace pitaya

#endif // PITAYA_SERVICE_DISCOVERY_H
