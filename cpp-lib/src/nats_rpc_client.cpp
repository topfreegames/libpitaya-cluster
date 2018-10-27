#include "pitaya_nats.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "nats/nats.h"
#include "pitaya.h"
#include <string>
#include <iostream>

using std::string;
using namespace pitaya;

pitaya_nats::NATSRPCClient::NATSRPCClient(std::shared_ptr<Server> server, std::shared_ptr<pitaya_nats::NATSConfig> config){
    natsStatus s;
    timeout_ms = config->request_timeout_ms;

    s = natsConnection_ConnectTo(&nc, config->nats_addr.c_str());

    if (s != NATS_OK) {
        throw new PitayaException("unable to initialize nats server");
    }
}

std::shared_ptr<protos::Response> pitaya_nats::NATSRPCClient::Call(std::shared_ptr<pitaya::Server> target, const string &route, const char * msg, const int msg_size){
    natsMsg *reply = NULL;
    natsStatus s;
    auto topic = GetTopicForServer(std::move(target));

    auto p_msg = new protos::Msg();
    p_msg->set_route(route);
    p_msg->set_data(msg);
    p_msg->set_type(protos::MsgType::MsgRequest);

    auto req = std::unique_ptr<protos::Request>(new protos::Request);
    req->set_type(protos::RPCType::User);
    req->set_allocated_msg(p_msg);
    size_t size = req->ByteSizeLong();
    void *buffer = malloc(size);
    req->SerializeToArray(buffer, size);

    s = natsConnection_Request(&reply, nc, topic.c_str(), buffer, size, timeout_ms);
    auto res = std::make_shared<protos::Response>();

    if (s != NATS_OK){
        auto err = new protos::Error();
        if (s == NATS_TIMEOUT){
            err->set_code("PIT-504");
            err->set_msg("nats timeout");
        } else{
            err->set_code("PIT-500");
            err->set_msg("nats error");
        }
        res->set_allocated_error(err);
    } else {
        res->set_data(natsMsg_GetData(reply));
    }

    natsMsg_Destroy(reply);
    free(buffer);

    return res;
}