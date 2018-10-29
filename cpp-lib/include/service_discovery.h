#ifndef TFG_SERVICE_DISCOVERY_H
#define TFG_SERVICE_DISCOVERY_H

#include <string>
#include <memory>
#include <chrono>
#include <functional>

#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include "sync_map.h"
#include "pitaya.h"
#include "spdlog/spdlog.h"

namespace service_discovery {

    using ServerId = std::string;
    using ServerType = std::string;

    enum class Action {
        Add, Remove
    };

    class ServiceDiscovery {
    public:
        ServiceDiscovery(std::shared_ptr<pitaya::Server> server, const std::string &address);

        std::shared_ptr<pitaya::Server> GetServerById(const std::string& id);
        std::vector<std::shared_ptr<pitaya::Server>> GetServersByType(const std::string &type);

    private:
        etcdv3::V3Status Init();
        void Configure();
        void OnWatch(etcd::Response res);
        void SyncServers();
        void AddServer(std::shared_ptr<pitaya::Server> server);
        void NotifyListeners(Action action, const pitaya::Server *server);

        std::shared_ptr<pitaya::Server> GetServerFromEtcd(const std::string &serverId, const std::string &serverType);

        etcdv3::V3Status Bootstrap();
        etcdv3::V3Status BootstrapServer(const pitaya::Server &server);
        etcdv3::V3Status AddServerToEtcd(const pitaya::Server &server);
        etcdv3::V3Status GrantLease();

        void DeleteLocalInvalidServers(const std::vector<std::string> &actualServers);
        void DeleteServer(const std::string &serverId);
        void PrintServers();

        std::shared_ptr<pitaya::Server> ParseServer(const std::string &jsonStr);
        bool ParseEtcdKey(const std::string &key, std::string &serverType, std::string &serverId);

        int64_t CreateLease();

    private:
        std::shared_ptr<spdlog::logger> _log;

        std::shared_ptr<pitaya::Server> _server;
        etcd::Client _client;
        etcd::Watcher _watcher;

        SyncMap<ServerId, std::shared_ptr<pitaya::Server>> _serversById;
        SyncMap<ServerType, std::unordered_map<std::string, std::shared_ptr<pitaya::Server>>> _serversByType;

        // Configuration member values
        std::chrono::seconds _syncServersInterval;
        std::chrono::seconds _heartbeatTTL;
        bool _logHeartbeat;
        std::chrono::time_point<std::chrono::system_clock> _lastHeartbeatTime;
        int64_t _leaseId;
        std::vector<std::string> _etcdEndpoints;
        std::string _etcdPrefix;
//        std::chrono::seconds _etcdDialTimeout;
        bool _running;
        std::chrono::time_point<std::chrono::system_clock> _lastSyncTime;
        std::chrono::seconds _revokeTimeout;
        std::chrono::seconds _grantLeaseTimeout;
        int _grantLeaseMaxRetries;
        std::chrono::seconds _grantLeaseInterval;
    };

} // namespace service_discovery

#endif // TFG_SERVICE_DISCOVERY_H
