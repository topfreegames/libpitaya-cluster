#include "pitaya_nats.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "nats/nats.h"
#include "pitaya.h"
#include <string>
#include <iostream>

using std::string;
using namespace pitaya;

pitaya_nats::NATSRPCClient::NATSRPCClient(std::shared_ptr<Server> server, std::shared_ptr<pitaya_nats::NATSConfig> config)
: nc(nullptr)
, sub(nullptr)
, timeout_ms(config->request_timeout_ms)
{
    natsStatus s = natsConnection_ConnectTo(&nc, config->nats_addr.c_str());
    if (s != NATS_OK) {
        throw PitayaException("unable to initialize nats server");
    }
}

std::shared_ptr<protos::Response> pitaya_nats::NATSRPCClient::Call(std::shared_ptr<pitaya::Server> target, std::unique_ptr<protos::Request> req)
{
    auto topic = GetTopicForServer(std::move(target));

    size_t size = req->ByteSizeLong();
    std::vector<uint8_t> buffer;
    buffer.reserve(size);

    req->SerializeToArray(buffer.data(), size);

    natsMsg *reply = nullptr;
    natsStatus s = natsConnection_Request(&reply, nc, topic.c_str(), buffer.data(), size, timeout_ms);
    auto res = std::make_shared<protos::Response>();

    if (s != NATS_OK){
        // TODO: verify the protobuf code, but this code seems to have a memory leak. Where is err deleted?
        auto err = new protos::Error();
        if (s == NATS_TIMEOUT){
            // TODO const codes in separate file
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

    return res;
}
