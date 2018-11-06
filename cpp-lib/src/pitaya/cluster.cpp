#include "pitaya/cluster.h"
#include "pitaya/utils.h"
#include "protos/msg.pb.h"

using namespace pitaya;
using namespace std;

using google::protobuf::MessageLite;

namespace pitaya {

Cluster::Cluster(service_discovery::Config&& sdConfig,
                 nats::NATSConfig&& natsConfig,
                 Server server,
                 RPCHandlerFunc rpcServerHandlerFunc,
                 const char* loggerName)
{
    _log =
        loggerName ? spdlog::get(loggerName)->clone("cluster") : spdlog::stdout_color_mt("cluster");
    _log->set_level(spdlog::level::debug);
    _natsConfig = std::move(natsConfig);
    _server = std::move(server);
    _rpcServerHandlerFunc = rpcServerHandlerFunc;

    _rpcSv = unique_ptr<nats::NATSRPCServer>(
        new nats::NATSRPCServer(_server, _natsConfig, _rpcServerHandlerFunc));
    _rpcClient = unique_ptr<nats::NATSRPCClient>(new nats::NATSRPCClient(_server, _natsConfig));
    _sd = std::unique_ptr<service_discovery::ServiceDiscovery>(
        new service_discovery::ServiceDiscovery(std::move(sdConfig), _server));

    _log->info("Cluster correctly initialized");
}

std::unique_ptr<pitaya::PitayaError>
Cluster::RPC(const string& route,
             std::shared_ptr<MessageLite> arg,
             std::shared_ptr<MessageLite> ret)
{
    try {
        auto r = pitaya::Route(route);
        auto sv_type = r.server_type;
        auto servers = _sd->GetServersByType(sv_type);
        if (servers.size() < 1) {
            auto err = std::unique_ptr<pitaya::PitayaError>(
                new pitaya::PitayaError("PIT-404", "no servers found for route: " + route));
            return err;
        }
        pitaya::Server sv = pitaya::utils::RandomServer(servers);
        return RPC(sv.id, route, arg, ret);
    } catch (PitayaException* e) {
        auto err =
            std::unique_ptr<pitaya::PitayaError>(new pitaya::PitayaError("PIT-500", e->what()));
        return err;
    }
}

std::unique_ptr<PitayaError>
Cluster::RPC(const string& server_id,
             const string& route,
             std::shared_ptr<MessageLite> arg,
             std::shared_ptr<MessageLite> ret)
{
    _log->debug("Calling RPC on server {}", server_id);
    auto sv = _sd->GetServerById(server_id);
    if (!sv) {
        // TODO better error code with constants somewhere
        auto err = std::unique_ptr<pitaya::PitayaError>(
            new pitaya::PitayaError("PIT-404", "server not found"));
        return err;
    }
    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route(route.c_str());

    size_t size = arg->ByteSizeLong();
    void* buffer = malloc(size);
    arg->SerializeToArray(buffer, size);
    msg->set_data((const char*)buffer);

    auto req = std::unique_ptr<protos::Request>(new protos::Request());
    req->set_type(protos::RPCType::User);
    req->set_allocated_msg(msg);

    auto res = _rpcClient->Call(sv.value(), std::move(req));
    if (res->has_error()) {
        _log->debug("Received error calling client rpc: {}", res->error().msg());
        auto err = std::unique_ptr<pitaya::PitayaError>(
            new pitaya::PitayaError(res->error().code(), res->error().msg()));
        return err;
    }

    _log->debug("Successfuly called rpc: {}", res->data());

    auto parsed = ret->ParseFromString(res->data());
    if (!parsed) {
        auto err = std::unique_ptr<pitaya::PitayaError>(
            new pitaya::PitayaError("PIT-500", "error parsing protobuf"));
        return err;
    }

    free(buffer);
    return nullptr;
}

} // namespace pitaya
