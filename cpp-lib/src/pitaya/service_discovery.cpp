#include "pitaya/service_discovery.h"
#include "pitaya/service_discovery/lease_keep_alive.h"
#include "pitaya/utils/string_utils.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <algorithm>
#include <cpprest/json.h>

using std::cerr;
using std::cout;
using std::endl;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::chrono::seconds;
namespace chrono = std::chrono;
namespace json = web::json;

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace pitaya;

namespace pitaya {
namespace service_discovery {

// Helper functions
ServiceDiscovery::ServiceDiscovery(const Server& server, const string& address)
    : _log(spdlog::stdout_color_mt("service_discovery"))
    , _worker(address, "pitaya/servers", server)
{
    if (server.id.empty() || server.type.empty()) {
        throw PitayaException("Server id and type cannot be empty");
    }

    _worker.WaitUntilInitialized();
    _log->set_level(spdlog::level::debug);
}

ServiceDiscovery::~ServiceDiscovery()
{
    _log->info("Terminating");
}

boost::optional<pitaya::Server>
ServiceDiscovery::GetServerById(const std::string& id)
{
    return _worker.GetServerById(id);
}

vector<Server>
ServiceDiscovery::GetServersByType(const std::string& type)
{
    return _worker.GetServersByType(type);
}

}
}
