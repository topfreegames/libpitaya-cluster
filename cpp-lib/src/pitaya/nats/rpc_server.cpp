#include "pitaya/nats/rpc_server.h"
#include "nats/nats.h"
#include "pitaya.h"
#include "pitaya/utils.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <boost/format.hpp>
#include <cstdio>
#include <iostream>

using std::string;
using namespace pitaya;

namespace pitaya {
namespace nats {

RPCHandlerFunc RPCServer::handler;

RPCServer::RPCServer(const Server& server,
                     const NATSConfig& config,
                     RPCHandlerFunc handlerFunc,
                     const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("nats_rpc_server")
                      : spdlog::stdout_color_mt("nats_rpc_server"))
    , _nc(nullptr)
    , _sub(nullptr)
{
    _log->set_level(spdlog::level::debug);
    natsOptions* opts;
    auto s = natsOptions_Create(&opts);
    if (s != NATS_OK) {
        throw PitayaException("error configuring nats server;");
    }
    natsOptions_SetTimeout(opts, config.connectionTimeoutMs);
    natsOptions_SetMaxReconnect(opts, config.maxReconnectionAttempts);
    natsOptions_SetMaxPendingMsgs(opts, config.maxPendingMsgs);
    natsOptions_SetClosedCB(opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(opts, ReconnectedCb, this);
    natsOptions_SetErrorHandler(opts, ErrHandler, this);
    natsOptions_SetURL(opts, config.natsAddr.c_str());

    handler = handlerFunc;
    s = natsConnection_Connect(&_nc, opts);
    if (s == NATS_OK) {
        s = natsConnection_Subscribe(
            &_sub, _nc, utils::GetTopicForServer(server).c_str(), HandleMsg, this);
    }
    if (s == NATS_OK) {
        _log->info("nats rpc server configured!");
    } else {
        throw PitayaException("unable to initialize nats server");
    }
}

void
RPCServer::HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    auto instance = reinterpret_cast<RPCServer*>(closure);

    instance->PrintSubStatus(sub);

    auto req = protos::Request();
    auto reply = natsMsg_GetReply(msg);
    bool decoded = req.ParseFromArray(natsMsg_GetData(msg), natsMsg_GetDataLength(msg));
    if (!decoded) {
        instance->_log->error("unable to decode msg from nats");
        return;
    }

    protos::Response res = handler(req);

    std::vector<uint8_t> buffer(res.ByteSizeLong());
    res.SerializeToArray(buffer.data(), buffer.size());

    natsConnection_Publish(nc, reply, buffer.data(), buffer.size());
    natsMsg_Destroy(msg);
}

void
RPCServer::PrintSubStatus(natsSubscription* subscription)
{
    int pendingMsgs;
    int maxPendingMsgs;
    int64_t deliveredMsgs;
    int64_t droppedMsgs;
    auto s = natsSubscription_GetStats(subscription,
                                       &pendingMsgs,
                                       nullptr,
                                       &maxPendingMsgs,
                                       nullptr,
                                       &deliveredMsgs,
                                       &droppedMsgs);
    _log->debug("nats server status: pending:{}, max_pending:{}, delivered:{}, dropped:{}",
                pendingMsgs,
                maxPendingMsgs,
                deliveredMsgs,
                droppedMsgs);
}

void
RPCServer::ErrHandler(natsConnection* nc,
                      natsSubscription* subscription,
                      natsStatus err,
                      void* closure)
{
    auto instance = (RPCServer*)closure;
    if (err == NATS_SLOW_CONSUMER) {
        instance->_log->error("nats runtime error: slow consumer");
    } else {
        instance->_log->error("nats runtime error: {}", err);
    }
}

void
RPCServer::DisconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<RPCServer*>(closure);
    instance->_log->error("nats disconnected! will try to reconnect...");
}

void
RPCServer::ReconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<RPCServer*>(closure);
    instance->_log->error("nats reconnected!");
}

void
RPCServer::ClosedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<RPCServer*>(closure);
    instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

} // namespace nats
} // namespace pitaya
