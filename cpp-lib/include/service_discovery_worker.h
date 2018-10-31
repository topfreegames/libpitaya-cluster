#ifndef SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H
#define SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H

#include <string>
#include <thread>
#include <chrono>
#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include "semaphore.h"
#include "sync_deque.h"
#include "sync_map.h"
#include "spdlog/spdlog.h"
#include "pitaya.h"
#include <boost/optional.hpp>
#include <pplx/pplxtasks.h>
#include "lease_keep_alive.h"

namespace service_discovery {

    enum class JobInfo {
        EtcdReconnectionFailure,
        WatchError,
    };

    class ServiceDiscoveryWorker {
    public:
        ServiceDiscoveryWorker(const std::string &etcdAddress,
                               const std::string &etcdPrefix,
                               pitaya::Server server);
        ~ServiceDiscoveryWorker();

        boost::optional<pitaya::Server> GetServerById(const std::string &id);
        std::vector<pitaya::Server> GetServersByType(const std::string &type);

    private:
        void StartThread();
        void OnWatch(etcd::Response res);
        etcdv3::V3Status Init();
        etcdv3::V3Status Bootstrap();
        etcdv3::V3Status GrantLease();
        etcdv3::V3Status BootstrapServer(const pitaya::Server &server);
        etcdv3::V3Status AddServerToEtcd(const pitaya::Server &server);
        void AddServer(const pitaya::Server &server);
        void SyncServers();
        void PrintServers();
        void DeleteServer(const std::string &serverId);
        bool ParseEtcdKey(const std::string &key, std::string &serverType, std::string &serverId);
        void PrintServer(const pitaya::Server &server);
        boost::optional<pitaya::Server> GetServerFromEtcd(const std::string &serverId, const std::string &serverType);
        void DeleteLocalInvalidServers(const std::vector<std::string> &actualServers);
        boost::optional<pitaya::Server> ParseServer(const std::string &jsonStr);

    private:
        pitaya::Server _server;
        etcd::Client _client;
        etcd::Watcher _watcher;
        std::string _etcdPrefix;
        int64_t _leaseId;
        std::shared_ptr<spdlog::logger> _log;
        std::thread _workerThread;

        std::chrono::seconds _leaseTTL;
        LeaseKeepAlive _leaseKeepAlive;
        int _numKeepAliveRetriesLeft;

        std::chrono::time_point<std::chrono::system_clock> _lastSyncTime;

        thread_utils::Semaphore _semaphore;
        SyncDeque<JobInfo> _jobQueue;
        SyncMap<std::string, pitaya::Server> _serversById;
        SyncMap<std::string, std::unordered_map<std::string, pitaya::Server>> _serversByType;
    };

} // namespace service_discovery

#endif // SERVICE_DISCOVERY_SERVICE_DISCOVERY_WORKER_H
