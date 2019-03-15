#include "pitaya/etcdv3_service_discovery.h"

#include "pitaya/etcdv3_service_discovery/lease_keep_alive.h"
#include "pitaya/utils/string_utils.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <algorithm>

using std::cerr;
using std::cout;
using std::endl;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::chrono::seconds;
namespace chrono = std::chrono;

using std::placeholders::_1;
using namespace pitaya;

namespace pitaya {
namespace etcdv3_service_discovery {

// Helper functions
Etcdv3ServiceDiscovery::Etcdv3ServiceDiscovery(const Config& config,
                                               const Server& server,
                                               const char* loggerName)
    : _log((loggerName && spdlog::get(loggerName))
               ? spdlog::get(loggerName)->clone("service_discovery")
               : spdlog::stdout_color_mt("service_discovery"))
    , _worker(config, server, loggerName ? loggerName : "service_discovery")
{
    if (server.id.empty() || server.type.empty()) {
        throw PitayaException("Server id and type cannot be empty");
    }

    _worker.WaitUntilInitialized();
}

Etcdv3ServiceDiscovery::~Etcdv3ServiceDiscovery()
{
    _log->info("Terminating");
    _log->flush();
    spdlog::drop("service_disovery");
}

boost::optional<pitaya::Server>
Etcdv3ServiceDiscovery::GetServerById(const std::string& id)
{
    return _worker.GetServerById(id);
}

vector<Server>
Etcdv3ServiceDiscovery::GetServersByType(const std::string& type)
{
    return _worker.GetServersByType(type);
}

void
Etcdv3ServiceDiscovery::AddListener(service_discovery::Listener* listener)
{
    _worker.AddListener(listener);
}

} // namespace etcdv3_service_discovery
} // namespace pitaya
