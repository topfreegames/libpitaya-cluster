#include "pitaya/cluster.h"

#include "pitaya/constants.h"
#include "pitaya/etcd_binding_storage.h"
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

using std::placeholders::_1;
using std::placeholders::_2;

using google::protobuf::MessageLite;

namespace pitaya {

using etcdv3_service_discovery::Etcdv3ServiceDiscovery;
using service_discovery::ServiceDiscovery;

void
Cluster::InitializeWithGrpc(GrpcConfig config,
                            etcdv3_service_discovery::Config&& sdConfig,
                            EtcdBindingStorageConfig bindingStorageConfig,
                            Server server,
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

    auto bindingStorage = std::unique_ptr<BindingStorage>(new EtcdBindingStorage(
        bindingStorageConfig,
        std::unique_ptr<EtcdClient>(new EtcdClientV3(
            bindingStorageConfig.endpoint, bindingStorageConfig.etcdPrefix, false, loggerName)),
        loggerName));

    Initialize(server,
               sd,
               std::unique_ptr<RpcServer>(new GrpcServer(
                   config, std::bind(&Cluster::OnIncomingRpc, this, _1, _2), loggerName)),
               std::unique_ptr<RpcClient>(
                   new GrpcClient(config,
                                  sd,
                                  std::move(bindingStorage),
                                  [](std::shared_ptr<grpc::ChannelInterface> channel)
                                      -> std::unique_ptr<protos::Pitaya::StubInterface> {
                                      return protos::Pitaya::NewStub(channel);
                                  },
                                  loggerName)));
}

void
Cluster::InitializeWithNats(NatsConfig&& natsConfig,
                            etcdv3_service_discovery::Config&& sdConfig,
                            Server server,
                            const char* loggerName)
{
    Initialize(
        server,
        std::shared_ptr<ServiceDiscovery>(new Etcdv3ServiceDiscovery(
            std::move(sdConfig),
            server,
            std::unique_ptr<EtcdClient>(new EtcdClientV3(
                sdConfig.endpoints, sdConfig.etcdPrefix, sdConfig.logHeartbeat, loggerName)),
            loggerName)),
        std::unique_ptr<RpcServer>(new nats::NatsRpcServer(
            server, natsConfig, std::bind(&Cluster::OnIncomingRpc, this, _1, _2), loggerName)),
        std::unique_ptr<RpcClient>(new NatsRpcClient(natsConfig, loggerName)));
}

void
Cluster::Initialize(Server server,
                    std::shared_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName)
{
    _log = utils::CloneLoggerOrCreate(loggerName, "cluster");
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
}

optional<PitayaError>
Cluster::RPC(const string& route, protos::Request& req, protos::Response& ret)
{
    try {
        auto r = pitaya::Route(route);
        auto sv_type = r.server_type;
        auto servers = _sd->GetServersByType(sv_type);
        if (servers.size() < 1) {
            return PitayaError(constants::kCodeNotFound, "no servers found for route: " + route);
        }
        pitaya::Server sv = pitaya::utils::RandomServer(servers);
        return RPC(sv.Id(), route, req, ret);
    } catch (PitayaException* e) {
        return PitayaError(constants::kCodeInternalError, e->what());
    }
}

optional<PitayaError>
Cluster::SendPushToUser(const string& serverId, const string& serverType, protos::Push& push)
{
    _log->debug("Sending push to user {} on server {}", push.uid(), serverId);

    auto error = _rpcClient->SendPushToUser(serverId, serverType, push);
    if (error) {
        _log->error("Received error sending push: {}", error.value().msg);
    } else {
        _log->debug("Successfuly sent push");
    }

    return boost::none;
}

optional<PitayaError>
Cluster::SendKickToUser(const string& serverId, const string& serverType, protos::KickMsg& kick)
{
    _log->debug("Sending kick to user {} on server {}", kick.userid(), serverId);

    auto error = _rpcClient->SendKickToUser(serverId, serverType, kick);
    if (!error) {
        _log->debug("successfuly sent kick");
    }

    return error;
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
        return PitayaError(constants::kCodeNotFound, "server not found");
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
        return PitayaError(ret.error().code(), ret.error().msg());
    }

    _log->debug("Successfuly called rpc: {}", ret.data());

    return boost::none;
}

void
Cluster::OnIncomingRpc(const protos::Request& req, Rpc* rpc)
{
    std::lock_guard<decltype(_waitingRpcs)> lock(_waitingRpcs);

    RpcData rpcData = {};
    rpcData.req = req;
    rpcData.rpc = rpc;

    _waitingRpcs.PushBack(rpcData);
    _waitingRpcsSemaphore.Notify();
}

Cluster::RpcData
Cluster::WaitForRpc()
{
    _waitingRpcsSemaphore.Wait();
    std::lock_guard<decltype(_waitingRpcs)> lock(_waitingRpcs);
    //_log->info("Will consume new rpc");

    // TODO: define better when there are no more rpc incoming
    if (_waitingRpcs.Empty()) {
        RpcData rpcData = {};
        rpcData.rpc = nullptr;
        return rpcData;
    } else {
        RpcData rpcData = _waitingRpcs.PopFront();
        return rpcData;
    }
}

} // namespace pitaya
