#include "pitaya/etcdv3_service_discovery.h"

#include "pitaya/etcdv3_service_discovery/worker.h"
#include "pitaya/utils.h"
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

static constexpr const char* kLogTag = "service_discovery";

namespace pitaya {
namespace etcdv3_service_discovery {

Etcdv3ServiceDiscovery::Etcdv3ServiceDiscovery(const EtcdServiceDiscoveryConfig& config,
                                               Server server,
                                               std::unique_ptr<EtcdClient> etcdClient,
                                               const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
{
    if (server.Id().empty() || server.Type().empty()) {
        throw PitayaException("Server id and type cannot be empty");
    }
    _worker = std::unique_ptr<Worker>(
        new Worker(config, server, std::move(etcdClient), loggerName ? loggerName : kLogTag));
    _worker->WaitUntilInitialized();
}

Etcdv3ServiceDiscovery::~Etcdv3ServiceDiscovery()
{
    _log->info("Terminating");
    _log->flush();
}

boost::optional<pitaya::Server>
Etcdv3ServiceDiscovery::GetServerById(const std::string& id)
{
    return _worker->GetServerById(id);
}

vector<Server>
Etcdv3ServiceDiscovery::GetServersByType(const std::string& type)
{
    return _worker->GetServersByType(type);
}

void
Etcdv3ServiceDiscovery::AddListener(service_discovery::Listener* listener)
{
    _worker->AddListener(listener);
}

void
Etcdv3ServiceDiscovery::RemoveListener(service_discovery::Listener* listener)
{
    _worker->RemoveListener(listener);
}

} // namespace etcdv3_service_discovery
} // namespace pitaya
