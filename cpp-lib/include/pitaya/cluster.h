#ifndef PITAYA_CLUSTER_H
#define PITAYA_CLUSTER_H

#include "pitaya.h"
#include "pitaya/etcd_config.h"
#include "pitaya/grpc_config.h"
#include "pitaya/nats_config.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_client.h"
#include "pitaya/rpc_server.h"
#include "pitaya/service_discovery.h"
#include "pitaya/utils/semaphore.h"
#include "pitaya/utils/sync_deque.h"
#include "spdlog/spdlog.h"

#include <boost/optional.hpp>
#include <google/protobuf/message_lite.h>
#include <ostream>

namespace pitaya {

class Cluster
{
public:
    static Cluster& Instance()
    {
        static Cluster instance;
        return instance;
    }

    void Initialize(Server server,
                    std::shared_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName = nullptr);

    void InitializeWithNats(NatsConfig natsConfig,
                            EtcdServiceDiscoveryConfig sdConfig,
                            Server server,
                            const char* loggerName = nullptr);

    void InitializeWithGrpc(GrpcConfig config,
                            EtcdServiceDiscoveryConfig sdConfig,
                            EtcdBindingStorageConfig bindingStorageConfig,
                            Server server,
                            const char* loggerName = nullptr);

    void Terminate();

    void AddServiceDiscoveryListener(service_discovery::Listener* listener);

    void RemoveServiceDiscoveryListener(service_discovery::Listener* listener);

    service_discovery::ServiceDiscovery& GetServiceDiscovery() { return *_sd.get(); }

    boost::optional<PitayaError> RPC(const std::string& serverId,
                                     const std::string& route,
                                     protos::Request& req,
                                     protos::Response& ret);

    boost::optional<PitayaError> RPC(const std::string& route,
                                     protos::Request& req,
                                     protos::Response& ret);

    boost::optional<PitayaError> SendPushToUser(const std::string& server_id,
                                                const std::string& server_type,
                                                protos::Push& push);

    boost::optional<PitayaError> SendKickToUser(const std::string& server_id,
                                                const std::string& server_type,
                                                protos::KickMsg& kick);

    struct RpcData
    {
        protos::Request req;
        Rpc* rpc;
    };

    boost::optional<RpcData> WaitForRpc();

private:
    void OnIncomingRpc(const protos::Request& req, Rpc* rpc);

private:
    std::shared_ptr<spdlog::logger> _log;
    std::shared_ptr<service_discovery::ServiceDiscovery> _sd;
    std::unique_ptr<RpcClient> _rpcClient;
    std::unique_ptr<RpcServer> _rpcSv;
    Server _server;

    utils::SyncDeque<RpcData> _waitingRpcs;
    utils::Semaphore _waitingRpcsSemaphore;
    bool _waitingRpcsFinished;
};

} // namespace pitaya

#endif // PITAYA_CLUSTER_H
