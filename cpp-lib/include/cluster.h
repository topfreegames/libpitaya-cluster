#ifndef TFG_CLUSTER_H
#define TFG_CLUSTER_H

#include <pitaya.h>
#include <pitaya_nats.h>
#include <service_discovery.h>
#include "protos/response.pb.h"
#include "protos/request.pb.h"
#include <google/protobuf/message_lite.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace pitaya
{

    struct PitayaError {
        std::string code;
        std::string msg;

        PitayaError(const std::string& code, const std::string& msg)
        : code(code), msg(msg)
        {}
    };

    class Cluster {
    public:
        Cluster(
            pitaya_nats::NATSConfig &nats_config,
            Server server,
            rpc_handler_func rpc_server_handler_func
        )
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
        pitaya_nats::NATSConfig nats_config;
        Server server;
        std::unique_ptr<service_discovery::ServiceDiscovery> sd;
        std::unique_ptr<pitaya_nats::NATSRPCServer> rpc_sv;
        std::unique_ptr<pitaya_nats::NATSRPCClient> rpc_client;
        rpc_handler_func rpc_server_handler_func = nullptr;
    };

} // namespace pitaya
#endif
