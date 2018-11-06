#include "pitaya/nats/rpc_client.h"
#include "nats/nats.h"
#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "pitaya/utils.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>
#include <string>

using std::string;
using namespace pitaya;

namespace pitaya {
namespace nats {

NATSRPCClient::NATSRPCClient(const Server& server, const NATSConfig& config, const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("nats_rpc_client")
                      : spdlog::stdout_color_mt("nats_rpc_client"))
    , nc(nullptr)
    , sub(nullptr)
    , timeout_ms(config.requestTimeoutMs)
{
    _log->set_level(spdlog::level::debug);
    natsOptions* opts;
    auto s = natsOptions_Create(&opts);
    if (s != NATS_OK) {
        throw PitayaException("error configuring nats server;");
    }
    natsOptions_SetTimeout(opts, config.connectionTimeoutMs);
    natsOptions_SetMaxReconnect(opts, config.maxReconnectionAttempts);
    natsOptions_SetClosedCB(opts, closed_cb, this);
    natsOptions_SetDisconnectedCB(opts, disconnected_cb, this);
    natsOptions_SetReconnectedCB(opts, reconnected_cb, this);
    natsOptions_SetURL(opts, config.natsAddr.c_str());

    s = natsConnection_Connect(&nc, opts);
    if (s != NATS_OK) {
        throw PitayaException("unable to initialize nats server");
    } else {
        _log->info("nats rpc client configured!");
    }
}

std::shared_ptr<protos::Response>
NATSRPCClient::Call(const pitaya::Server& target, std::unique_ptr<protos::Request> req)
{
    auto topic = utils::GetTopicForServer(target);

    size_t size = req->ByteSizeLong();
    std::vector<uint8_t> buffer;
    buffer.reserve(size);

    req->SerializeToArray(buffer.data(), size);

    natsMsg* reply = nullptr;
    natsStatus s =
        natsConnection_Request(&reply, nc, topic.c_str(), buffer.data(), size, timeout_ms);
    auto res = std::make_shared<protos::Response>();

    if (s != NATS_OK) {
        // TODO: verify the protobuf code, but this code seems to have a memory leak.
        // Where is err deleted?
        auto err = new protos::Error();
        if (s == NATS_TIMEOUT) {
            // TODO const codes in separate file
            err->set_code("PIT-504");
            err->set_msg("nats timeout");
        } else {
            err->set_code("PIT-500");
            err->set_msg("nats error");
        }
        res->set_allocated_error(err);
    } else {
        res->ParseFromArray(natsMsg_GetData(reply), natsMsg_GetDataLength(reply));
    }

    natsMsg_Destroy(reply);

    return res;
}

void
NATSRPCClient::disconnected_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCClient*)closure;
    instance->_log->error("nats disconnected! will try to reconnect...");
}

void
NATSRPCClient::reconnected_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCClient*)closure;
    instance->_log->error("nats reconnected!");
}

void
NATSRPCClient::closed_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCClient*)closure;
    instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

}
}
