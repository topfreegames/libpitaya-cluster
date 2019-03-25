#include "pitaya/nats/rpc_client.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/nats/config.h"
#include "pitaya/utils.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>
#include <nats/nats.h>
#include <string>

using std::string;
using namespace pitaya;

static constexpr const char* kLogTag = "nats_rpc_client";

namespace pitaya {
namespace nats {

NatsRpcClient::NatsRpcClient(const NatsConfig& config, const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _opts(nullptr)
    , _nc(nullptr)
    , _timeoutMs(config.requestTimeoutMs)
{
    auto s = natsOptions_Create(&_opts);
    if (s != NATS_OK) {
        throw PitayaException("error configuring nats server;");
    }
    natsOptions_SetTimeout(_opts, config.connectionTimeoutMs);
    natsOptions_SetMaxReconnect(_opts, config.maxReconnectionAttempts);
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

    s = natsConnection_Connect(&_nc, _opts);
    if (s != NATS_OK) {
        natsOptions_Destroy(_opts);
        throw PitayaException("unable to initialize nats server");
    } else {
        _log->info("nats rpc client configured!");
    }
}

NatsRpcClient::~NatsRpcClient()
{
    _log->info("Stopping rpc client");
    natsConnection_Destroy(_nc);
    natsOptions_Destroy(_opts);

    _log->info("rpc client shut down");
    _log->flush();
    spdlog::drop(kLogTag);
}

protos::Response
NatsRpcClient::Call(const pitaya::Server& target, const protos::Request& req)
{
    auto topic = utils::GetTopicForServer(target);

    std::vector<uint8_t> buffer(req.ByteSizeLong());
    req.SerializeToArray(buffer.data(), buffer.size());

    natsMsg* reply = nullptr;
    natsStatus s = natsConnection_Request(
        &reply, _nc, topic.c_str(), buffer.data(), buffer.size(), _timeoutMs);
    protos::Response res;

    if (s != NATS_OK) {
        auto err = new protos::Error();
        if (s == NATS_TIMEOUT) {
            // TODO const codes in separate file
            err->set_code(constants::kCodeTimeout);
            err->set_msg("nats timeout");
        } else {
            err->set_code(constants::kCodeInternalError);
            err->set_msg("nats error");
        }
        res.set_allocated_error(err);
    } else {
        res.ParseFromArray(natsMsg_GetData(reply), natsMsg_GetDataLength(reply));
    }

    natsMsg_Destroy(reply);
    return res;
}

void
NatsRpcClient::DisconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcClient*>(closure);
    instance->_log->error("nats disconnected! will try to reconnect...");
}

void
NatsRpcClient::ReconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcClient*>(closure);
    instance->_log->error("nats reconnected!");
}

void
NatsRpcClient::ClosedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcClient*>(closure);
    instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

protos::Response
NatsRpcClient::SendPushToUser(const std::string& user_id, const std::string& server_id, const std::string& server_type, const protos::Push& push){
{
    //TODO implement here
    protos::Response res;
    auto err = new protos::Error();
    err->set_code(pitaya::constants::kCodeNotImplemented);
    err->set_msg("method not implemented");
    res.set_allocated_error(err);

    return res;
}
}

} // namespace nats
} // namespace pitaya
