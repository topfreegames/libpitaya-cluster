#ifndef PITAYA_GRPC_RPC_CLIENT_H
#define PITAYA_GRPC_RPC_CLIENT_H

#include "pitaya/rpc_client.h"
#include "pitaya/service_discovery.h"
#include "pitaya/utils/sync_map.h"
#include "protos/pitaya.grpc.pb.h"
#include "spdlog/spdlog.h"

#include <grpcpp/channel.h>
#include <unordered_map>

namespace pitaya {

class GrpcClient
    : public RpcClient
    , public service_discovery::Listener
{
public:
    GrpcClient(std::shared_ptr<service_discovery::ServiceDiscovery> serviceDiscovery,
               const pitaya::Server& server,
               const char* loggerName = nullptr);
    protos::Response Call(const pitaya::Server& target, const protos::Request& req) override;

    void ServerAdded(const pitaya::Server& server) override;
    void ServerRemoved(const pitaya::Server& server) override;

private:
    std::shared_ptr<spdlog::logger> _log;
    Server _server;
    std::shared_ptr<service_discovery::ServiceDiscovery> _serviceDiscovery;
    utils::SyncMap<std::string, std::unique_ptr<protos::Pitaya::Stub>> _stubsForServers;
};

} // namespace pitaya

#endif // PITAYA_GRPC_RPC_CLIENT_H
