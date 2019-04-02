#include "pitaya/nats/rpc_server.h"

#include "pitaya.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/utils.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include "nats/nats.h"
#include <boost/format.hpp>
#include <cstdio>
#include <iostream>

using std::string;
using namespace pitaya;

static constexpr const char* kLogTag = "nats_rpc_server";

namespace pitaya {
namespace nats {

std::atomic_int NatsRpcServer::_cnt;

NatsRpcServer::NatsRpcServer(const Server& server,
                             const NatsConfig& config,
                             RpcHandlerFunc handlerFunc,
                             const char* loggerName)
    : RpcServer(handlerFunc)
    , _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _opts(nullptr)
    , _nc(nullptr)
    , _sub(nullptr)
{
    auto s = natsOptions_Create(&_opts);
    if (s != NATS_OK) {
        throw PitayaException("error configuring nats server;");
    }
    natsOptions_SetTimeout(_opts, config.connectionTimeoutMs);
    natsOptions_SetMaxReconnect(_opts, config.maxReconnectionAttempts);
    natsOptions_SetMaxPendingMsgs(_opts, config.maxPendingMsgs);
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    natsOptions_SetErrorHandler(_opts, ErrHandler, this);
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

    s = natsConnection_Connect(&_nc, _opts);
    if (s == NATS_OK) {
        s = natsConnection_Subscribe(&_sub,
                                     _nc,
                                     utils::GetTopicForServer(server.Id(), server.Type()).c_str(),
                                     HandleMsg,
                                     this);
        _log->info("nats rpc server configured!");
    } else {
        natsOptions_Destroy(_opts);
        throw PitayaException("unable to initialize nats server");
    }
}

NatsRpcServer::~NatsRpcServer()
{
    _log->info("Stopping rpc server");
    natsSubscription_Destroy(_sub);
    natsConnection_Destroy(_nc);
    natsOptions_Destroy(_opts);

    _log->info("rpc server shut down");
    _log->flush();
    spdlog::drop(kLogTag);
}

class CallData : public pitaya::Rpc
{
public:
    CallData(natsConnection* nc, const char* reply, natsMsg* msg)
        : _reply(reply)
        , _nc(nc)
        , _msg(msg)
    {
        assert(nc);
        assert(reply);
    }

    void Finish(protos::Response res) override
    {
        std::vector<uint8_t> buffer(res.ByteSizeLong());
        res.SerializeToArray(buffer.data(), buffer.size());

        natsConnection_Publish(_nc, _reply, buffer.data(), buffer.size());
        natsMsg_Destroy(_msg);
        delete this;
    }

private:
    const char* _reply;
    natsConnection* _nc;
    natsMsg* _msg;
};

void
NatsRpcServer::HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcServer*>(closure);

    if (_cnt == 10000) {
        instance->PrintSubStatus(sub);
        _cnt = 0;
    }

    _cnt++;

    auto req = protos::Request();
    auto reply = natsMsg_GetReply(msg);
    bool decoded = req.ParseFromArray(natsMsg_GetData(msg), natsMsg_GetDataLength(msg));
    if (!decoded) {
        instance->_log->error("unable to decode msg from nats");
        return;
    }

    instance->_handlerFunc(req, new CallData(nc, reply, msg));
}

void
NatsRpcServer::PrintSubStatus(natsSubscription* subscription)
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
NatsRpcServer::ErrHandler(natsConnection* nc,
                          natsSubscription* subscription,
                          natsStatus err,
                          void* closure)
{
    auto instance = (NatsRpcServer*)closure;
    if (err == NATS_SLOW_CONSUMER) {
        instance->_log->error("nats runtime error: slow consumer");
    } else {
        instance->_log->error("nats runtime error: {}", err);
    }
}

void
NatsRpcServer::DisconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcServer*>(closure);
    instance->_log->error("nats disconnected! will try to reconnect...");
}

void
NatsRpcServer::ReconnectedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcServer*>(closure);
    instance->_log->error("nats reconnected!");
}

void
NatsRpcServer::ClosedCb(natsConnection* nc, void* closure)
{
    auto instance = reinterpret_cast<NatsRpcServer*>(closure);
    instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

} // namespace nats
} // namespace pitaya
