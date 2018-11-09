#ifndef PITAYA_MOCK_SERVICE_DISCOVERY_H
#define PITAYA_MOCK_SERVICE_DISCOVERY_H

#include <gmock/gmock.h>
#include <pitaya/service_discovery.h>

class MockServiceDiscovery : public pitaya::service_discovery::ServiceDiscovery
{
public:
    MOCK_METHOD1(GetServerById, boost::optional<pitaya::Server>(const std::string& id));
    MOCK_METHOD1(GetServersByType, std::vector<pitaya::Server>(const std::string& type));
}

#endif // PITAYA_MOCK_SERVICE_DISCOVERY_H
