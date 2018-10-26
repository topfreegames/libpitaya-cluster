#include <iostream>
#include <memory>
#include <service_discovery.h>

using namespace std;
using service_discovery::ServiceDiscovery;

std::unique_ptr<ServiceDiscovery> gServiceDiscovery;

int
main()
{
    auto server = unique_ptr<service_discovery::Server>(new service_discovery::Server);

    gServiceDiscovery = std::unique_ptr<ServiceDiscovery>(
        new ServiceDiscovery(std::move(server), "http://127.0.0.1:4001")
    );
}
