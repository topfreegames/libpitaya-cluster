#ifndef SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H
#define SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H

#include "pitaya.h"
#include "pitaya/service_discovery/lease_keep_alive.h"
#include "pitaya/utils/semaphore.h"
#include "pitaya/utils/sync_deque.h"
#include "pitaya/utils/sync_map.h"
#include "pitaya/utils/ticker.h"
#include "spdlog/spdlog.h"
#include <boost/optional.hpp>
#include <chrono>
#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include <pplx/pplxtasks.h>
#include <string>
#include <thread>

namespace pitaya {
namespace service_discovery {

enum class JobInfo
{
    EtcdReconnectionFailure,
    WatchError,
    Shutdown,
};

class Worker
{
public:
    Worker(const std::string& etcdAddress, const std::string& etcdPrefix, pitaya::Server server);
    ~Worker();

    boost::optional<pitaya::Server> GetServerById(const std::string& id);
    std::vector<pitaya::Server> GetServersByType(const std::string& type);
    void WaitUntilInitialized();

private:
    void Shutdown();
    void StartThread();
    void OnWatch(etcd::Response res);
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

private:
    bool _workerExiting;

    std::promise<void> _initPromise;
    std::string _etcdPrefix;
    pitaya::Server _server;
    etcd::Client _client;
    etcd::Watcher _watcher;
    int64_t _leaseId;
    std::shared_ptr<spdlog::logger> _log;
    std::thread _workerThread;

    std::chrono::seconds _leaseTTL;
    LeaseKeepAlive _leaseKeepAlive;
    int _numKeepAliveRetriesLeft;

    std::chrono::seconds _syncServersInterval;
    utils::Ticker _syncServersTicker;

    utils::Semaphore _semaphore;
    utils::SyncDeque<JobInfo> _jobQueue;
    utils::SyncMap<std::string, pitaya::Server> _serversById;
    utils::SyncMap<std::string, std::unordered_map<std::string, pitaya::Server>> _serversByType;
};

}
}

#endif // SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H
