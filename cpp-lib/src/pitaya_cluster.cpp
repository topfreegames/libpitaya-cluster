#include <iostream>
#include <memory>
#include <service_discovery.h>
#include <pitaya.h>

using namespace std;
using service_discovery::ServiceDiscovery;

std::unique_ptr<ServiceDiscovery> gServiceDiscovery;

int
main()
{
    auto server = unique_ptr<pitaya::Server>(new pitaya::Server);

    gServiceDiscovery = std::unique_ptr<ServiceDiscovery>(
        new ServiceDiscovery(std::move(server), "http://127.0.0.1:4001")
    );
}
