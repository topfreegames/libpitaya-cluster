#ifndef PITAYA_ETCDV3_SERVICE_DISCOVERY_WORKER_H
#define PITAYA_ETCDV3_SERVICE_DISCOVERY_WORKER_H

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/etcdv3_service_discovery/lease_keep_alive.h"
#include "pitaya/service_discovery.h"
#include "pitaya/utils/semaphore.h"
#include "pitaya/utils/sync_deque.h"
#include "pitaya/utils/sync_map.h"
#include "pitaya/utils/sync_vector.h"
#include "pitaya/utils/ticker.h"
#include "spdlog/spdlog.h"
#include <atomic>
#include <boost/optional.hpp>
#include <chrono>
#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include <pplx/pplxtasks.h>
#include <string>
#include <thread>

namespace pitaya {
namespace etcdv3_service_discovery {

enum class JobInfo
{
    EtcdReconnectionFailure,
    WatchError,
    Shutdown,
};

class Worker
{
public:
    Worker(const Config& config, pitaya::Server server, const char* loggerName);
    ~Worker();

    boost::optional<pitaya::Server> GetServerById(const std::string& id);
    std::vector<pitaya::Server> GetServersByType(const std::string& type);
    void WaitUntilInitialized();
    void AddListener(service_discovery::Listener* listener);

private:
    void Shutdown();
    void StartThread();
    void OnWatch(etcd::Response res);
    void StartLeaseKeepAlive();
    etcdv3::V3Status Init();
    etcdv3::V3Status Bootstrap();
    etcdv3::V3Status GrantLease();
    etcdv3::V3Status BootstrapServer(const pitaya::Server& server);
    etcdv3::V3Status AddServerToEtcd(const pitaya::Server& server);
    void AddServer(const pitaya::Server& server);
    void SyncServers();
    void PrintServers();
    void DeleteServer(const std::string& serverId);
    bool ParseEtcdKey(const std::string& key, std::string& serverType, std::string& serverId);
    void PrintServer(const pitaya::Server& server);
    void RevokeLease();
    boost::optional<pitaya::Server> GetServerFromEtcd(const std::string& serverId,
                                                      const std::string& serverType);
    void DeleteLocalInvalidServers(const std::vector<std::string>& actualServers);
    boost::optional<pitaya::Server> ParseServer(const std::string& jsonStr);

    void BroadcastServerAdded(const pitaya::Server& server);
    void BroadcastServerRemoved(const pitaya::Server& server);

private:
    Config _config;
    std::atomic_bool _workerExiting;

    std::promise<void> _initPromise;
    pitaya::Server _server;
    etcd::Client _client;
    etcd::Watcher _watcher;
    int64_t _leaseId;
    std::shared_ptr<spdlog::logger> _log;
    std::thread _workerThread;

    LeaseKeepAlive _leaseKeepAlive;
    int _numKeepAliveRetriesLeft;

    utils::Ticker _syncServersTicker;

    utils::Semaphore _semaphore;
    utils::SyncDeque<JobInfo> _jobQueue;
    utils::SyncMap<std::string, pitaya::Server> _serversById;
    utils::SyncMap<std::string, std::unordered_map<std::string, pitaya::Server>> _serversByType;

    utils::SyncVector<service_discovery::Listener*> _listeners;
};

} // namespace etcdv3_service_discovery
} // namespace pitaya

#endif // PITAYA_ETCDV3_SERVICE_DISCOVERY_WORKER_H
