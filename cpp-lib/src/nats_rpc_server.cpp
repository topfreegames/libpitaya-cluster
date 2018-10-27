#include <cstdio>
#include <iostream>
#include <boost/format.hpp>
#include "pitaya_nats.h"
#include "nats/nats.h"
#include "pitaya.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"

using std::string;
using std::cout;
using namespace pitaya;

// TODO: configure rpc server
pitaya_nats::NATSRPCServer::NATSRPCServer(std::shared_ptr<Server> server, std::shared_ptr<pitaya_nats::NATSConfig> config)
{
    natsStatus s;
    s = natsConnection_ConnectTo(&nc, config->nats_addr.c_str());
    if (s == NATS_OK) {
        s = natsConnection_Subscribe(&sub, nc, 
            GetTopicForServer(std::move(server)).c_str(), handleMsg, NULL);
    }
    if (s == NATS_OK){
        cout << "nats configured!" << std::endl;
    } else {
        throw new PitayaException("unable to initialize nats server");
    }
}

void pitaya_nats::NATSRPCServer::handleMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure){
    // Prints the message, using the message getters:
    auto req = std::unique_ptr<protos::Request>(new protos::Request());
    auto reply = natsMsg_GetReply(msg);
    bool decoded = req->ParseFromArray(
        natsMsg_GetData(msg), natsMsg_GetDataLength(msg));
    if (!decoded) {
        cout << "unable to decode msg from nats" << std::endl; 
        return;
    }
    auto rpc_req = std::unique_ptr<pitaya::RPCReq>(new pitaya::RPCReq());
    rpc_req->data = req->msg().data().c_str();
    rpc_req->data_len = req->msg().data().length();
    rpc_req->route = req->msg().route().c_str();
    cout << "request with route:" << rpc_req->route <<
    " data_len:" << rpc_req->data_len << std::endl;

    //TODO call callback, get res

    auto res = std::unique_ptr<protos::Response>(new protos::Response());
    res->set_data("ok!");

    size_t size = res->ByteSizeLong();
    void *buffer = malloc(size);
    res->SerializeToArray(buffer, size);

    natsConnection_Publish(nc, reply, buffer, size);

    natsMsg_Destroy(msg);
    free(buffer);
}