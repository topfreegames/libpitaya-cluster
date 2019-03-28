#include "pitaya/cluster.h"

#include "pitaya/constants.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"
#include "pitaya/protos/msg.pb.h"
#include "pitaya/utils.h"

#include <cpprest/json.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace pitaya;
using namespace std;
namespace json = web::json;
using boost::optional;

using google::protobuf::MessageLite;

namespace pitaya {

using etcdv3_service_discovery::Etcdv3ServiceDiscovery;
using service_discovery::ServiceDiscovery;

void
Cluster::InitializeWithGrpc(GrpcConfig config,
                            etcdv3_service_discovery::Config&& sdConfig,
                            Server server,
                            RpcHandlerFunc rpcServerHandlerFunc,
                            const char* loggerName)
{
    // In order to other servers know how to connect to our grpc server,
    // we need to publish our host and port as metadata of the server.
    // This needs to happen before the ServiceDiscovery is created.
    server.WithMetadata(constants::kGrpcHostKey, config.host)
        .WithMetadata(constants::kGrpcPortKey, std::to_string(config.port));

    auto sd = std::shared_ptr<ServiceDiscovery>(new Etcdv3ServiceDiscovery(
        std::move(sdConfig),
        server,
        std::unique_ptr<EtcdClient>(new EtcdClientV3(
            sdConfig.endpoints, sdConfig.etcdPrefix, sdConfig.logHeartbeat, loggerName)),
        loggerName));

    Initialize(server,
               sd,
               std::unique_ptr<RpcServer>(new GrpcServer(config, rpcServerHandlerFunc, loggerName)),
               std::unique_ptr<RpcClient>(
                   new GrpcClient(config,
                                  sd,
                                  [](std::shared_ptr<grpc::ChannelInterface> channel)
                                      -> std::unique_ptr<protos::Pitaya::StubInterface> {
                                      return protos::Pitaya::NewStub(channel);
                                  },
                                  loggerName)));
}

void
Cluster::InitializeWithNats(nats::NatsConfig&& natsConfig,
                            etcdv3_service_discovery::Config&& sdConfig,
                            Server server,
                            RpcHandlerFunc rpcServerHandlerFunc,
                            const char* loggerName)
{
    Initialize(server,
               std::shared_ptr<ServiceDiscovery>(new Etcdv3ServiceDiscovery(
                   std::move(sdConfig),
                   server,
                   std::unique_ptr<EtcdClient>(new EtcdClientV3(
                       sdConfig.endpoints, sdConfig.etcdPrefix, sdConfig.logHeartbeat, loggerName)),
                   loggerName)),
               std::unique_ptr<RpcServer>(
                   new nats::NatsRpcServer(server, natsConfig, rpcServerHandlerFunc, loggerName)),
               std::unique_ptr<RpcClient>(new nats::NatsRpcClient(natsConfig, loggerName)));
}

void
Cluster::Initialize(Server server,
                    std::shared_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName)
{
    _log = ((loggerName && spdlog::get(loggerName)) ? spdlog::get(loggerName)->clone("cluster")
                                                    : spdlog::stdout_color_mt("cluster"));
    _sd = std::move(sd);
    _rpcSv = std::move(rpcServer);
    _rpcClient = std::move(rpcClient);
    _server = server;
}

void
Cluster::Terminate()
{
    if (_log) {
        _log->flush();
    }
    _sd.reset();
    _rpcClient.reset();
    _rpcSv.reset();
    if (spdlog::get("cluster")) {
        spdlog::drop("cluster");
    }
}

optional<PitayaError>
Cluster::RPC(const string& route, protos::Request& req, protos::Response& ret)
{
    try {
        auto r = pitaya::Route(route);
        auto sv_type = r.server_type;
        auto servers = _sd->GetServersByType(sv_type);
        if (servers.size() < 1) {
            return pitaya::PitayaError(constants::kCodeNotFound,
                                       "no servers found for route: " + route);
        }
        pitaya::Server sv = pitaya::utils::RandomServer(servers);
        return RPC(sv.Id(), route, req, ret);
    } catch (PitayaException* e) {
        return pitaya::PitayaError(constants::kCodeInternalError, e->what());
    }
}

optional<PitayaError>
Cluster::SendPushToUser(const string& server_id,
                        const string& server_type,
                        protos::Push& push,
                        protos::Response& ret)
{
    _log->debug("Sending push to user {} on server {}", push.uid(), server_id);
    // TODO if no server id sent still goq
    auto sv = _sd->GetServerById(server_id);
    if (!sv) {
        _log->error("Did not find server id {}", server_id);
        return pitaya::PitayaError(constants::kCodeNotFound, "server not found");
    }

    ret = _rpcClient->SendPushToUser(sv.value().Id(), sv.value().Type(), push);
    if (ret.has_error()) {
        _log->error("Received error sending push: {}", ret.error().msg());
        return pitaya::PitayaError(ret.error().code(), ret.error().msg());
    }

    _log->debug("Successfuly sent push: {}", ret.data());

    return boost::none;
}

optional<PitayaError>
Cluster::SendKickToUser(const string& server_id,
                        const string& server_type,
                        protos::KickMsg& kick,
                        protos::KickAnswer& ret)
{
    _log->debug("Sending kick to user {} on server {}", kick.userid(), server_id);
    // TODO if no server id sent still go
    auto sv = _sd->GetServerById(server_id);
    if (!sv) {
        _log->error("Did not find server id {}", server_id);
        return pitaya::PitayaError(constants::kCodeNotFound, "server not found");
    }

    ret = _rpcClient->SendKickToUser(sv.value().Id(), sv.value().Type(), kick);
    _log->debug("successfuly sent kick: kicked: {}", ret.kicked());

    return boost::none;
}

optional<PitayaError>
Cluster::RPC(const string& server_id,
             const string& route,
             protos::Request& req,
             protos::Response& ret)
{
    _log->debug("Calling RPC on server {}", server_id);
    auto sv = _sd->GetServerById(server_id);
    if (!sv) {
        _log->error("Did not find server id {}", server_id);
        return pitaya::PitayaError(constants::kCodeNotFound, "server not found");
    }

    // TODO proper jaeger setup
    json::value metadata;
    metadata.object();
    metadata[constants::kPeerIdKey] = json::value::string(_server.Id());
    metadata[constants::kPeerServiceKey] = json::value::string(_server.Type());
    string metadataStr = metadata.serialize();
    req.set_metadata(metadataStr);

    ret = _rpcClient->Call(sv.value(), req);
    if (ret.has_error()) {
        _log->error("Received error calling client rpc: {}", ret.error().msg());
        return pitaya::PitayaError(ret.error().code(), ret.error().msg());
    }

    _log->debug("Successfuly called rpc: {}", ret.data());

    return boost::none;
}

} // namespace pitaya
