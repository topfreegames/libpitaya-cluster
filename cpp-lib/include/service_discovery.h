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

namespace service_discovery {

    using ServerId = std::string;
    using ServerType = std::string;

    struct Listener {
        std::function<void(pitaya::Server*)> addServer;
        std::function<void(pitaya::Server*)> removeServer;
    };

    class ServiceDiscovery {
    public:
        ServiceDiscovery(std::shared_ptr<pitaya::Server> server, const std::string &address);

        void Init();
        void AddListener(const Listener &listener);
        std::shared_ptr<pitaya::Server> GetServerByID(const string& id);

    private:
        void Configure();
        void OnWatch(etcd::Response res);
        void SyncServers();
        void AddServer(std::shared_ptr<pitaya::Server> server);
        std::shared_ptr<pitaya::Server> GetServerFromEtcd(const std::string &serverId, const std::string &serverType);

        etcdv3::V3Status BootstrapServer(const pitaya::Server &server);
        etcdv3::V3Status AddServerToEtcd(const pitaya::Server &server);

        int64_t CreateLease();

    private:
        std::shared_ptr<pitaya::Server> _server;

        etcd::Client _client;
        etcd::Watcher _watcher;

        SyncMap<ServerId, std::shared_ptr<pitaya::Server>> _serversById;
        SyncMap<ServerType, std::unordered_map<std::string, std::shared_ptr<pitaya::Server>>> _serversByType;

        std::vector<Listener> _listeners;

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
