#ifndef TFG_SERVICE_DISCOVERY_H
#define TFG_SERVICE_DISCOVERY_H

#include <string>
#include <memory>
#include <chrono>
#include <functional>

#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include "sync_map.h"

namespace service_discovery {

    using ServerId = std::string;

    struct Server {
        std::string id;
        std::string type;
        std::string metadata;
        std::string hostname;
        bool frontend;

        std::string GetKey() const
        {
            // TODO: Add fmt library to improve this code.
            return std::string("servers") + std::string("/") + type + std::string("/") + id;
        }
    };

    struct Listener {
        std::function<void(Server*)> addServer;
        std::function<void(Server*)> removeServer;
    };

    class ServiceDiscovery {
    public:
        ServiceDiscovery(std::unique_ptr<Server> server, const std::string &address);

        void Init();
        void AddListener(const Listener &listener);

    private:
        void Configure();
        void OnWatch(etcd::Response res);
        void SyncServers();

        etcdv3::V3Status BootstrapServer(const Server &server);
        etcdv3::V3Status AddServerToEtcd(const Server &server);

        int64_t CreateLease();

    private:
        std::unique_ptr<Server> _server;

        etcd::Client _client;
        etcd::Watcher _watcher;

        SyncMap<ServerId, std::shared_ptr<Server>> _serversById;

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
