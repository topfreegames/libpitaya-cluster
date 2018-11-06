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

RPCHandlerFunc NATSRPCServer::handler;

NATSRPCServer::NATSRPCServer(const Server& server,
                             const NATSConfig& config,
                             RPCHandlerFunc handler_func,
                             const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone("nats_rpc_server")
                      : spdlog::stdout_color_mt("nats_rpc_server"))
    , nc(nullptr)
    , sub(nullptr)
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
    natsOptions_SetClosedCB(opts, closed_cb, this);
    natsOptions_SetDisconnectedCB(opts, disconnected_cb, this);
    natsOptions_SetReconnectedCB(opts, reconnected_cb, this);
    natsOptions_SetErrorHandler(opts, err_handler, this);
    natsOptions_SetURL(opts, config.natsAddr.c_str());

    handler = handler_func;
    s = natsConnection_Connect(&nc, opts);
    if (s == NATS_OK) {
        s = natsConnection_Subscribe(
            &sub, nc, utils::GetTopicForServer(std::move(server)).c_str(), handle_msg, this);
    }
    if (s == NATS_OK) {
        _log->info("nats rpc server configured!");
    } else {
        throw PitayaException("unable to initialize nats server");
    }
}

void
NATSRPCServer::handle_msg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    auto instance = (NATSRPCServer*)closure;

    instance->print_sub_status(sub);

    auto req = protos::Request();
    auto reply = natsMsg_GetReply(msg);
    bool decoded = req.ParseFromArray(natsMsg_GetData(msg), natsMsg_GetDataLength(msg));
    if (!decoded) {
        instance->_log->error("unable to decode msg from nats");
        return;
    }

    protos::Response res = handler(req);

    size_t size = res.ByteSizeLong();
    void* buffer = malloc(size);
    res.SerializeToArray(buffer, size);

    natsConnection_Publish(nc, reply, buffer, size);

    natsMsg_Destroy(msg);
    free(buffer);
}

void
NATSRPCServer::print_sub_status(natsSubscription* subscription)
{
    int pending_msgs;
    int max_pending_msgs;
    int64_t delivered_msgs;
    int64_t dropped_msgs;
    auto s = natsSubscription_GetStats(subscription,
                                       &pending_msgs,
                                       nullptr,
                                       &max_pending_msgs,
                                       nullptr,
                                       &delivered_msgs,
                                       &dropped_msgs);
    _log->debug("nats server status: pending:{}, max_pending:{}, delivered:{}, dropped:{}",
                pending_msgs,
                max_pending_msgs,
                delivered_msgs,
                dropped_msgs);
}

void
NATSRPCServer::err_handler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* closure)
{
    auto instance = (NATSRPCServer*)closure;
    if (err == NATS_SLOW_CONSUMER) {
        instance->_log->error("nats runtime error: slow consumer");
    } else {
        instance->_log->error("nats runtime error: {}", err);
    }
}

void
NATSRPCServer::disconnected_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCServer*)closure;
    instance->_log->error("nats disconnected! will try to reconnect...");
}

void
NATSRPCServer::reconnected_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCServer*)closure;
    instance->_log->error("nats reconnected!");
}

void
NATSRPCServer::closed_cb(natsConnection* nc, void* closure)
{
    auto instance = (NATSRPCServer*)closure;
    instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

}
}
