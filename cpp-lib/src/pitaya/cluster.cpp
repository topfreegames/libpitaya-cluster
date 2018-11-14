#include "pitaya/cluster.h"
#include "pitaya/constants.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"
#include "pitaya/utils.h"
#include "protos/msg.pb.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <jaegertracing/Tracer.h>

using namespace pitaya;
using namespace std;
using boost::optional;

using google::protobuf::MessageLite;

namespace pitaya {

using etcdv3_service_discovery::Etcdv3ServiceDiscovery;
using service_discovery::ServiceDiscovery;

void Cluster::Initialize(etcdv3_service_discovery::Config&& sdConfig,
                         nats::NatsConfig&& natsConfig,
                         Server server,
                         RpcHandlerFunc rpcServerHandlerFunc,
                         const char* loggerName)
{
    Initialize(std::unique_ptr<ServiceDiscovery>(new Etcdv3ServiceDiscovery(std::move(sdConfig), server, loggerName)),
               std::unique_ptr<RpcServer>(new nats::NatsRpcServer(server, natsConfig, rpcServerHandlerFunc, loggerName)),
               std::unique_ptr<RpcClient>(new nats::NatsRpcClient(server, natsConfig, loggerName)));
}

void
    Cluster::Initialize(std::unique_ptr<service_discovery::ServiceDiscovery> sd,
                 std::unique_ptr<RpcServer> rpcServer,
                 std::unique_ptr<RpcClient> rpcClient,
                 const char* loggerName)
{
    _log = ((loggerName && spdlog::get(loggerName)) ? spdlog::get(loggerName)->clone("cluster")
            : spdlog::stdout_color_mt("cluster"));
    _log->set_level(spdlog::level::debug);
    _sd = std::move(sd);
    _rpcSv = std::move(rpcServer);
    _rpcClient = std::move(rpcClient);

    auto span1 = opentracing::Tracer::Global()->StartSpan("tracedFunction");
    auto span2 = opentracing::Tracer::Global()->StartSpan(
            "traced", { opentracing::ChildOf(&span1->context()) });
}

Cluster::~Cluster()
{
    Terminate();
}

void
Cluster::Terminate()
{
    _sd.reset();
    _rpcSv.reset();
    _rpcClient.reset();

    _log->flush();
    spdlog::drop("cluster");
}

optional<PitayaError>
Cluster::RPC(const string& route, const MessageLite& arg, MessageLite& ret)
{
    try {
        auto r = pitaya::Route(route);
        auto sv_type = r.server_type;
        auto servers = _sd->GetServersByType(sv_type);
        if (servers.size() < 1) {
            return pitaya::PitayaError(kCodeNotFound, "no servers found for route: " + route);
        }
        pitaya::Server sv = pitaya::utils::RandomServer(servers);
        return RPC(sv.id, route, arg, ret);
    } catch (PitayaException* e) {
        return pitaya::PitayaError(kCodeInternalError, e->what());
    }
}

optional<PitayaError>
Cluster::RPC(const string& server_id, const string& route, const MessageLite& arg, MessageLite& ret)
{
    _log->debug("Calling RPC on server {}", server_id);
    auto sv = _sd->GetServerById(server_id);
    if (!sv) {
        // TODO better error code with constants somewhere
        return pitaya::PitayaError(kCodeNotFound, "server not found");
    }
    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route(route.c_str());

    std::vector<uint8_t> buffer(arg.ByteSizeLong());
    arg.SerializeToArray(buffer.data(), buffer.size());
    msg->set_data(std::string((char*)buffer.data(), buffer.size()));

    protos::Request req;
    req.set_type(protos::RPCType::User);
    req.set_allocated_msg(msg);

    auto res = _rpcClient->Call(sv.value(), req);
    if (res.has_error()) {
        _log->debug("Received error calling client rpc: {}", res.error().msg());
        return pitaya::PitayaError(res.error().code(), res.error().msg());
    }

    _log->debug("Successfuly called rpc: {}", res.data());

    auto parsed = ret.ParseFromString(res.data());
    if (!parsed) {
        return pitaya::PitayaError(kCodeInternalError, "error parsing protobuf");
    }

    return boost::none;
}

} // namespace pitaya
