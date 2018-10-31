#include "service_discovery.h"
#include <cpprest/json.h>
#include "string_utils.h"
#include <algorithm>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "lease_keep_alive.h"

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::cout;
using std::cerr;
using std::endl;
using std::chrono::seconds;
namespace chrono = std::chrono;
namespace json = web::json;

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace pitaya;

// Helper functions
service_discovery::ServiceDiscovery::ServiceDiscovery(Server server, const string &address)
: _log(spdlog::stdout_color_mt("service_discovery"))
, _worker(address, "pitaya/servers", std::move(server))
{
    _log->set_level(spdlog::level::debug);
}

service_discovery::ServiceDiscovery::~ServiceDiscovery()
{
    _log->info("Terminating");
}

boost::optional<pitaya::Server>
service_discovery::ServiceDiscovery::GetServerById(const std::string &id)
{
    return _worker.GetServerById(id);
}

vector<Server>
service_discovery::ServiceDiscovery::GetServersByType(const std::string &type)
{
    return _worker.GetServersByType(type);
}
