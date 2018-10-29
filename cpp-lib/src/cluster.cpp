#include "cluster.h"
#include "protos/msg.pb.h"
#include <pitaya_nats.h>
#include <service_discovery.h>

using namespace pitaya_nats;
using namespace service_discovery;
using namespace std;

bool pitaya::Cluster::Init(){
    try{
        rpc_sv = unique_ptr<NATSRPCServer>(
            new NATSRPCServer(server, nats_config, rpc_server_handler_func));
        rpc_client = unique_ptr<NATSRPCClient>(
            new NATSRPCClient(server, nats_config));
        sd = std::unique_ptr<ServiceDiscovery>(
            new ServiceDiscovery(server, "http://127.0.0.1:4001")
        );
        return true;
    } catch(PitayaException* e){
        _log->error("error initializing cluster: {}", e->what());
        return false;
    }
}

std::unique_ptr<pitaya::PitayaError> pitaya::Cluster::RPC(const string& route, std::shared_ptr<MessageLite> arg, std::shared_ptr<MessageLite> ret){
    //return RPC();
    return NULL;
}

std::unique_ptr<pitaya::PitayaError> pitaya::Cluster::RPC(const string& server_id, const string& route, std::shared_ptr<MessageLite> arg, std::shared_ptr<MessageLite> ret){
    auto sv = sd->GetServerById(server_id);
    if (sv == nullptr){
        // TODO better error code with constants somewhere
        auto err = std::unique_ptr<pitaya::PitayaError>(new pitaya::PitayaError(
            "PIT-404", "server not found"));
        return err;
    }
    auto msg = new protos::Msg();
    msg->set_type(protos::MsgType::MsgRequest);
    msg->set_route(route.c_str());

    size_t size = arg->ByteSizeLong(); 
    void *buffer = malloc(size);
    arg->SerializeToArray(buffer, size);
    msg->set_data((const char *)buffer);

    auto req = std::unique_ptr<protos::Request>(new protos::Request());
    req->set_type(protos::RPCType::User);
    req->set_allocated_msg(msg);

    auto res = rpc_client->Call(sv, std::move(req));
    if(res->has_error()){
        auto err = std::unique_ptr<pitaya::PitayaError>(new pitaya::PitayaError(
            res->error().code(), res->error().msg()));
        return err;
    }

    auto parsed = ret->ParseFromString(res->data());
    if(!parsed){
        auto err = std::unique_ptr<pitaya::PitayaError>(new pitaya::PitayaError(
            "PIT-500", "error parsing protobuf"));
        return err;
    }

    free(buffer);
    return NULL;
}
