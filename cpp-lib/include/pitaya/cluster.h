#ifndef PITAYA_CLUSTER_H
#define PITAYA_CLUSTER_H

#include "pitaya.h"
#include "pitaya/etcdv3_service_discovery/config.h"
#include "pitaya/grpc/config.h"
#include "pitaya/nats/config.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_client.h"
#include "pitaya/rpc_server.h"
#include "pitaya/service_discovery.h"
#include "spdlog/spdlog.h"

#include <boost/optional.hpp>
#include <google/protobuf/message_lite.h>
#include <ostream>

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

template<class CharType, class CharTrait>
inline std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& os, const PitayaError& e)
{
    if (os.good()) {
        os << "PitayaError{ code = " << e.code << ", msg = " << e.msg << " }";
    }
    return os;
}

class Cluster
{
public:
    static Cluster& Instance()
    {
        static Cluster instance;
        return instance;
    }

    std::shared_ptr<RpcServer> _rpcSv;
    void Initialize(Server server,
                    std::shared_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName = nullptr);

    void InitializeWithNats(NatsConfig&& natsConfig,
                            etcdv3_service_discovery::Config&& sdConfig,
                            Server server,
                            RpcHandlerFunc rpcServerHandlerFunc,
                            const char* loggerName = nullptr);

    void InitializeWithGrpc(GrpcConfig config,
                            etcdv3_service_discovery::Config&& sdConfig,
                            Server server,
                            RpcHandlerFunc rpcServerHandlerFunc,
                            const char* loggerName = nullptr);

    void Terminate();

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
                                                protos::Push& push,
                                                protos::Response& ret);

    boost::optional<PitayaError> SendKickToUser(const std::string& server_id,
                                                const std::string& server_type,
                                                protos::KickMsg& kick,
                                                protos::KickAnswer& ret);

private:
    std::shared_ptr<spdlog::logger> _log;
    std::shared_ptr<service_discovery::ServiceDiscovery> _sd;
    std::unique_ptr<RpcClient> _rpcClient;
    Server _server;
};

} // namespace pitaya

#endif // PITAYA_CLUSTER_H
