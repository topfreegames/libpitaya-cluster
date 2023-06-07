#include "pitaya/nats_client.h"

#include "pitaya/utils.h"

#include <string>

namespace pitaya {

//
// NatsMsgImpl
//
NatsMsgImpl::NatsMsgImpl(natsMsg* msg)
    : _msg(msg)
{}

NatsMsgImpl::~NatsMsgImpl()
{
    if (_msg) {
        natsMsg_Destroy(_msg);
    }
}

const uint8_t*
NatsMsgImpl::GetData() const
{
    return reinterpret_cast<const uint8_t*>(natsMsg_GetData(_msg));
}

size_t
NatsMsgImpl::GetSize() const
{
    return natsMsg_GetDataLength(_msg);
}

const char*
NatsMsgImpl::GetReply() const
{
    return natsMsg_GetReply(_msg);
}

static constexpr const char* kLogTag = "nats_client";

//
// NatsClientImpl
//
NatsClientImpl::NatsClientImpl(NatsApiType apiType,
                               const NatsConfig& config,
                               const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _opts(nullptr)
    , _conn(nullptr)
    , _sub(nullptr)
    , _connClosed(false)
{
    if (config.natsAddr.empty()) {
        throw PitayaException("NATS address should not be empty");
    }

    if (config.maxReconnectionAttempts < 0) {
        throw PitayaException("NATS max reconnection attempts should be positive");
    }

    natsStatus status = natsOptions_Create(&_opts);
    if (status != NATS_OK) {
        std::string err_str("error configuring nats client - ");
        err_str.append(natsStatus_GetText(status));
        throw PitayaException(err_str);
    }

    natsOptions_SetTimeout(_opts, config.connectionTimeout.count());
    natsOptions_SetMaxReconnect(_opts, config.maxReconnectionAttempts);
    natsOptions_SetReconnectWait(_opts, 2000);
    natsOptions_SetReconnectBufSize(_opts, 8*1024*1024);
    natsOptions_SetRetryOnFailedConnect(_opts, true, NULL, this);
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    natsOptions_SetPingInterval(opts, 3);
    natsOptions_SetMaxPingsOut(opts, 3);
    if (apiType == NatsApiType::Asynchronous) {
        natsOptions_SetMaxPendingMsgs(_opts, config.maxPendingMsgs);
        natsOptions_SetErrorHandler(_opts, ErrHandler, this);
    }
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

    _log->info("NATS Connection Timeout - " + std::to_string(config.connectionTimeout.count()));
    _log->info("NATS Max Reconnect Attempts - " + std::to_string(config.maxReconnectionAttempts));
    _log->info("NATS Reconnect Wait Time - " + std::to_string(2000) + " //currently hardcoded ");
    _log->info("NATS Reconnect Buffer Size - " + std::to_string(8*1024*1024) + " //currently hardcoded");

    // TODO, FIXME: Change so that connection happens NOT in the constructor.
    status = natsConnection_Connect(&_conn, _opts);
    if (status != NATS_OK) {
        std::string err_str("unable to initialize nats server - ");
        err_str.append(natsStatus_GetText(status));
        throw PitayaException(err_str);
    }
}
    
NatsClientImpl::~NatsClientImpl()
{
    if (_sub) {
        // Remove interest from the subscription. Note that pending message may still
        // be received by the client.
        natsStatus status;
        status = natsSubscription_Unsubscribe(_sub);
        if (status != NATS_OK) {
            _log->error("Failed to unsubscribe");
        }
        
        status = natsSubscription_Drain(_sub);
        if (status != NATS_OK) {
            _log->error("Failed to drain subscription");
        }
        
        status =
            natsSubscription_WaitForDrainCompletion(_sub, _subscriptionDrainTimeout.count());
        if (status != NATS_OK) {
            _log->error("Failed to wait for subscription drain");
        }
        // Called only here, because it needs to wait for the natsSubscription_WaitForDrainCompletion
        natsSubscription_Destroy(_sub);
    }

    natsConnection_Close(_conn);
    while (!_connClosed) {
        // Wait until the connection is actually closed. This will be reported on a different
        // thread.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    natsConnection_Destroy(_conn);
    natsOptions_Destroy(_opts);
}

natsStatus
NatsClientImpl::Request(std::shared_ptr<NatsMsg>* msg,
                        const std::string& topic,
                        const std::vector<uint8_t>& data,
                        std::chrono::milliseconds timeout)
{
    natsMsg* reply = nullptr;
    natsStatus status = natsConnection_Request(
        &reply, _conn, topic.c_str(), data.data(), data.size(), timeout.count());

    if (status == NATS_OK) {
        *msg = std::shared_ptr<NatsMsg>(new NatsMsgImpl(reply));
        return status;
    }

    if (status == NATS_TIMEOUT) {
        return status;
    } else {
        return status;
    }
}

natsStatus
NatsClientImpl::Subscribe(const std::string& topic,
                          std::function<void(std::shared_ptr<NatsMsg>)> onMessage)
{
    _log->info("Subscribing to topic {}", topic);
    _onMessage = std::move(onMessage);
    natsStatus status = natsConnection_Subscribe(&_sub, _conn, topic.c_str(), HandleMsg, this);
    if (status != NATS_OK) {
        _log->error("Failed to subscribe");
        return status;
    }
    return status;
}

natsStatus
NatsClientImpl::Publish(const char* reply, const std::vector<uint8_t>& buf)
{
    natsStatus status = natsConnection_Publish(_conn, reply, buf.data(), buf.size());

    if (status != NATS_OK) {
        if (status == NATS_TIMEOUT) {
            return status;
        } else {
            return status;
        }
    }

    return status;
}

void
NatsClientImpl::HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* user)
{
    auto natsClient = static_cast<NatsClientImpl*>(user);
    assert(natsClient->_onMessage);
    natsClient->_onMessage(std::shared_ptr<NatsMsg>(new NatsMsgImpl(msg)));
}

void
NatsClientImpl::DisconnectedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // TODO: implement logic here
     instance->_log->warn("nats disconnected");
}

void
NatsClientImpl::ReconnectedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // TODO: implement logic here
    instance->_log->info("nats reconnected!");
}

void
NatsClientImpl::ClosedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // Signal main thread that the connection was actually closed
    instance->_log->info("nats connection closed!");
    instance->_connClosed = true;
}

void
NatsClientImpl::ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    if (err == NATS_SLOW_CONSUMER) {
        instance->_log->error("nats runtime error: slow consumer");
    } else {
        instance->_log->error("nats runtime error: {}", err);
    }
}

} // namespace pitaya
