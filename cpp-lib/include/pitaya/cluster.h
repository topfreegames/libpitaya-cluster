#ifndef PITAYA_CLUSTER_H
#define PITAYA_CLUSTER_H

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/nats/config.h"
#include "pitaya/rpc_client.h"
#include "pitaya/rpc_server.h"
#include "pitaya/service_discovery.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/spdlog.h"
#include <boost/optional.hpp>
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
    static Cluster& Instance()
    {
        static Cluster instance;
        return instance;
    }

    void Initialize(std::unique_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName = nullptr);

    void Initialize(etcdv3_service_discovery::Config&& sdConfig,
                    nats::NatsConfig&& natsConfig,
                    Server server,
                    RpcHandlerFunc rpcServerHandlerFunc,
                    const char* loggerName = nullptr);

    void Terminate();

    ~Cluster();

    service_discovery::ServiceDiscovery& GetServiceDiscovery() { return *_sd.get(); }

    boost::optional<PitayaError> RPC(const std::string& serverId,
                                     const std::string& route,
                                     const google::protobuf::MessageLite& arg,
                                     google::protobuf::MessageLite& ret);

    boost::optional<PitayaError> RPC(const std::string& route,
                                     const google::protobuf::MessageLite& arg,
                                     google::protobuf::MessageLite& ret);

private:
    std::shared_ptr<spdlog::logger> _log;
    std::unique_ptr<service_discovery::ServiceDiscovery> _sd;
    std::unique_ptr<RpcServer> _rpcSv;
    std::unique_ptr<RpcClient> _rpcClient;
};

} // namespace pitaya

#endif // PITAYA_CLUSTER_H
