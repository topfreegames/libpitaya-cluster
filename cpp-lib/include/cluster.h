#ifndef TFG_CLUSTER_H
#define TFG_CLUSTER_H

#include <pitaya.h>
#include <pitaya_nats.h>
#include <service_discovery.h>
#include "protos/response.pb.h"
#include "protos/request.pb.h"
#include <google/protobuf/message_lite.h>

using google::protobuf::MessageLite;

namespace pitaya
{

    struct PitayaError{
        std::string code;
        std::string msg;

        PitayaError(const string& code, const string& msg):
        code(code), msg(msg){};
    };

    class Cluster
    {
        public:
        Cluster(
            std::shared_ptr<pitaya_nats::NATSConfig> nats_config,
            std::shared_ptr<Server> server,
            rpc_handler_func rpc_server_handler_func
            ): nats_config(std::move(nats_config)),
            server(std::move(server)),
            rpc_server_handler_func(rpc_server_handler_func){};
        bool Init();
        std::unique_ptr<PitayaError> RPC(const string& server_id, const string& route, std::shared_ptr<MessageLite> arg, std::shared_ptr<MessageLite> ret);
        std::unique_ptr<PitayaError> RPC(const string& route, std::shared_ptr<MessageLite> arg, std::shared_ptr<MessageLite> ret);

        private:
        std::shared_ptr<pitaya_nats::NATSConfig> nats_config = NULL;
        std::shared_ptr<Server> server = NULL;
        std::unique_ptr<service_discovery::ServiceDiscovery> sd = NULL;
        std::unique_ptr<pitaya_nats::NATSRPCServer> rpc_sv = NULL;
        std::unique_ptr<pitaya_nats::NATSRPCClient> rpc_client = NULL;
        rpc_handler_func rpc_server_handler_func = NULL;
    };

} // namespace pitaya
#endif
