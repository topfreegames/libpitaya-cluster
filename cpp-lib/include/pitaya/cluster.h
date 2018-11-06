#ifndef PITAYA_CLUSTER_H
#define PITAYA_CLUSTER_H

#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"
#include "pitaya/service_discovery.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <google/protobuf/message_lite.h>

namespace pitaya {

struct PitayaError
{
    std::string code;
    std::string msg;

    PitayaError(const std::string& code, const std::string& msg)
        : code(code)
        , msg(msg)
    {}
};

class Cluster
{
public:
    Cluster(service_discovery::Config&& sdConfig,
            nats::NATSConfig&& natsConfig,
            Server server,
            RPCHandlerFunc rpcServerHandlerFunc,
            const char* loggerName = nullptr);

    service_discovery::ServiceDiscovery& GetServiceDiscovery() { return *_sd.get(); }

    std::unique_ptr<PitayaError> RPC(const std::string& serverId,
                                     const std::string& route,
                                     std::shared_ptr<google::protobuf::MessageLite> arg,
                                     std::shared_ptr<google::protobuf::MessageLite> ret);

    std::unique_ptr<PitayaError> RPC(const std::string& route,
                                     std::shared_ptr<google::protobuf::MessageLite> arg,
                                     std::shared_ptr<google::protobuf::MessageLite> ret);

private:
    std::shared_ptr<spdlog::logger> _log;
    pitaya::nats::NATSConfig _natsConfig;
    Server _server;
    std::unique_ptr<service_discovery::ServiceDiscovery> _sd;
    std::unique_ptr<nats::NATSRPCServer> _rpcSv;
    std::unique_ptr<nats::NATSRPCClient> _rpcClient;
    RPCHandlerFunc _rpcServerHandlerFunc;
};

} // namespace pitaya

#endif // PITAYA_CLUSTER_H
