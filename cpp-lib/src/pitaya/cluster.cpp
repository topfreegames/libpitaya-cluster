#include "pitaya/cluster.h"

#include "pitaya/constants.h"
#include "pitaya/etcd_binding_storage.h"
#include "pitaya/etcd_client_v3.h"
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
                            EtcdServiceDiscoveryConfig sdConfig,
                            EtcdBindingStorageConfig bindingStorageConfig,
                            Server server,
                            const char* loggerName)
{
    assert(!sdConfig.endpoints.empty());
    assert(!bindingStorageConfig.endpoint.empty());

    // In order to other servers know how to connect to our grpc server,
    // we need to publish our host and port as metadata of the server.
    // This needs to happen before the ServiceDiscovery is created.
    server.WithMetadata(constants::kGrpcHostKey, config.host)
        .WithMetadata(constants::kGrpcPortKey, std::to_string(config.port));

    // NOTE: we want to start the RPC server before the service discovery. This is necessary,
    // because as soon as we register the server in the etcd, other servers can start calling it,
    // therefore maybe calling a server that it is not started yet.
    auto rpcServer = std::unique_ptr<RpcServer>(new GrpcServer(config, loggerName));

    auto bindingStorage = std::unique_ptr<BindingStorage>(new EtcdBindingStorage(
        bindingStorageConfig,
        std::unique_ptr<EtcdClient>(new EtcdClientV3(
            bindingStorageConfig.endpoint, bindingStorageConfig.etcdPrefix, false, loggerName)),
        loggerName));

    auto serviceDiscovery = std::shared_ptr<ServiceDiscovery>(new Etcdv3ServiceDiscovery(
        sdConfig,
        server,
        std::unique_ptr<EtcdClient>(new EtcdClientV3(
            sdConfig.endpoints, sdConfig.etcdPrefix + "servers/metagame/", sdConfig.logHeartbeat, loggerName)),
        loggerName));

    auto rpcClient = std::unique_ptr<RpcClient>(
        new GrpcClient(config,
                       serviceDiscovery,
                       std::move(bindingStorage),
                       [](std::shared_ptr<grpc::ChannelInterface> channel)
                           -> std::unique_ptr<protos::Pitaya::StubInterface> {
                           return protos::Pitaya::NewStub(channel);
                       },
                       loggerName));

    Initialize(server, serviceDiscovery, std::move(rpcServer), std::move(rpcClient), loggerName);
}

void
Cluster::InitializeWithNats(NatsConfig natsConfig,
                            EtcdServiceDiscoveryConfig sdConfig,
                            Server server,
                            const char* loggerName)
{
    // NOTE: we want to start the RPC server before the service discovery. This is necessary,
    // because as soon as we register the server in the etcd, other servers can start calling it,
    // therefore maybe calling a server that it is not started yet.
    auto rpcServer = std::unique_ptr<RpcServer>(new NatsRpcServer(server, natsConfig, loggerName));
    auto rpcClient = std::unique_ptr<RpcClient>(new NatsRpcClient(natsConfig, loggerName));
    auto etcdClient = std::unique_ptr<EtcdClient>(new EtcdClientV3(
        sdConfig.endpoints, sdConfig.etcdPrefix + "servers/metagame/", sdConfig.logHeartbeat, loggerName));
    auto serviceDiscovery = std::shared_ptr<ServiceDiscovery>(
        new Etcdv3ServiceDiscovery(std::move(sdConfig), server, std::move(etcdClient), loggerName));

    Initialize(server, std::move(serviceDiscovery), std::move(rpcServer), std::move(rpcClient), loggerName);
}

void
Cluster::Initialize(Server server,
                    std::shared_ptr<service_discovery::ServiceDiscovery> sd,
                    std::unique_ptr<RpcServer> rpcServer,
                    std::unique_ptr<RpcClient> rpcClient,
                    const char* loggerName)
{
    _log = utils::CloneLoggerOrCreate(loggerName, "cluster");
    _waitingRpcsFinished = false;
    _sd = std::move(sd);
    _rpcSv = std::move(rpcServer);
    _rpcClient = std::move(rpcClient);
    _server = server;
    // NOTE: Not destroying the semaphore at the Terminate func due to the fact that threads
    // may still be running and using it. Then, it will surely crash.
    _waitingRpcsSemaphore.reset(new utils::Semaphore());

    _rpcSv->Start(std::bind(&Cluster::OnIncomingRpc, this, _1, _2));
}

void
Cluster::Terminate()
{
    if (_log) {
        _log->flush();
    }
    _sd.reset();
    _rpcClient.reset();
    if (_rpcSv) {
        _rpcSv->Shutdown();
        _rpcSv.reset();
    }
    _log.reset();
}

void
Cluster::AddServiceDiscoveryListener(service_discovery::Listener* listener)
{
    _sd->AddListener(listener);
}

void
Cluster::RemoveServiceDiscoveryListener(service_discovery::Listener* listener)
{
    _sd->RemoveListener(listener);
}

boost::optional<PitayaError>
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

boost::optional<PitayaError>
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

boost::optional<PitayaError>
Cluster::SendKickToUser(const string& serverId, const string& serverType, protos::KickMsg& kick)
{
    _log->debug("Sending kick to user {} on server {}", kick.userid(), serverId);

    auto error = _rpcClient->SendKickToUser(serverId, serverType, kick);
    if (!error) {
        _log->debug("successfuly sent kick");
    }

    return error;
}

boost::optional<PitayaError>
Cluster::RPC(const string& serverId,
             const string& route,
             protos::Request& req,
             protos::Response& ret)
{
    _log->debug("Calling RPC on server {}", serverId);
    auto sv = _sd->GetServerById(serverId);
    if (!sv) {
        _log->error("Did not find server id {}", serverId);
        return PitayaError(constants::kCodeNotFound, "server not found");
    }

    // TODO proper jaeger setup
    json::value metadata;
    metadata.object();
    metadata[constants::kPeerIdKey] = json::value::string(_server.Id());
    metadata[constants::kPeerServiceKey] = json::value::string(_server.Type());
    string metadataStr = metadata.serialize();
    req.set_metadata(metadataStr);

    pitaya::Server server = sv.value();
    ret = _rpcClient->Call(sv.value(), req);
    if (ret.has_error()) {
        _log->error("Received error calling client rpc for server id->{} hostname->{} on route->{} : {}",serverId, server.Hostname(), route, ret.error().msg());
        return PitayaError(ret.error().code(), ret.error().msg());
    } else {
        _log->info("RPC to server {} succeeded", serverId);
    }

    _log->debug("Successfuly called rpc: {}", ret.data());

    return boost::none;
}

void
Cluster::OnIncomingRpc(const protos::Request& req, Rpc* rpc)
{
    std::lock_guard<decltype(_waitingRpcs)> lock(_waitingRpcs);

    assert(_waitingRpcsFinished == false);

    if (rpc) {
        RpcData rpcData = {};
        rpcData.req = req;
        rpcData.rpc = rpc;
        _waitingRpcs.PushBack(rpcData);
        _waitingRpcsSemaphore->Notify();
    } else {
        // TODO, FIXME: intead of incrementing the count to 2000 here,
        // solve this in a more elegant way.
        _waitingRpcsFinished = true;
        _waitingRpcsSemaphore->NotifyAll(2000);
    }
}

boost::optional<Cluster::RpcData>
Cluster::WaitForRpc()
{
    // TODO: there are probably too many locks being used here.
    // After adding some benchmarks, research a better way of doing this.
    // (e.g., merging the semaphore and queue, atomics, etc.)
    _waitingRpcsSemaphore->Wait();

    std::lock_guard<decltype(_waitingRpcs)> lock(_waitingRpcs);

    if (_waitingRpcs.Size() > 0) {
        // There are still rpcs to process, so return one.
        RpcData rpcData = _waitingRpcs.PopFront();
        return rpcData;
    }

    // There are no more RPCs in the queue. Since the thread was woken up,
    // it means that the queue was finished.
    if (_waitingRpcsFinished) {
        return boost::none;
    }

    _log->warn("The waiting rpcs queue is empty but the queue is not finished!");
    return boost::none;
}

} // namespace pitaya
