#ifndef PITAYA_CLUSTER_H
#define PITAYA_CLUSTER_H

#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "pitaya/nats/rpc_client.h"
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
    Cluster(pitaya::nats::NATSConfig& nats_config,
            Server server,
            rpc_handler_func rpc_server_handler_func)
        : _log(spdlog::stdout_color_mt("cluster"))
        , nats_config(nats_config)
        , server(std::move(server))
        , rpc_server_handler_func(rpc_server_handler_func)
    {}

    bool Init();

    std::unique_ptr<PitayaError> RPC(const std::string& server_id,
                                     const std::string& route,
                                     std::shared_ptr<google::protobuf::MessageLite> arg,
                                     std::shared_ptr<google::protobuf::MessageLite> ret);

    std::unique_ptr<PitayaError> RPC(const std::string& route,
                                     std::shared_ptr<google::protobuf::MessageLite> arg,
                                     std::shared_ptr<google::protobuf::MessageLite> ret);

private:
    std::shared_ptr<spdlog::logger> _log;
    pitaya::nats::NATSConfig nats_config;
    Server server;
    std::unique_ptr<service_discovery::ServiceDiscovery> sd;
    std::unique_ptr<nats::NATSRPCServer> rpc_sv;
    std::unique_ptr<nats::NATSRPCClient> rpc_client;
    rpc_handler_func rpc_server_handler_func;
};

} // namespace pitaya

#endif // PITAYA_CLUSTER_H
