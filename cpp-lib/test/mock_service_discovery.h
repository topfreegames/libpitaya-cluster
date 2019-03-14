#ifndef PITAYA_MOCK_SERVICE_DISCOVERY_H
#define PITAYA_MOCK_SERVICE_DISCOVERY_H

#include "trompeloeil.hpp"
#include <pitaya/service_discovery.h>

class MockServiceDiscovery : public pitaya::service_discovery::ServiceDiscovery
{
public:
    MAKE_MOCK1(GetServerById, boost::optional<pitaya::Server>(const std::string& id), override);
    MAKE_MOCK1(GetServersByType, std::vector<pitaya::Server>(const std::string& type), override);
    MAKE_MOCK1(AddListener, void(pitaya::service_discovery::Listener* listener), override);
};

#endif // PITAYA_MOCK_SERVICE_DISCOVERY_H
