#include "pitaya/nats_client.h"

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

//
// NatsClientImpl
//
NatsClientImpl::NatsClientImpl(NatsApiType apiType, const NatsConfig& config)
    : _opts(nullptr)
    , _conn(nullptr)
{
    if (config.natsAddr.empty()) {
        throw PitayaException("NATS address should not be empty");
    }

    if (config.maxReconnectionAttempts < 0) {
        throw PitayaException("NATS max reconnection attempts should be positive");
    }

    natsStatus status = natsOptions_Create(&_opts);
    if (status != NATS_OK) {
        throw PitayaException("error configuring nats client");
    }

    natsOptions_SetTimeout(_opts, config.connectionTimeout.count());
    natsOptions_SetMaxReconnect(_opts, config.maxReconnectionAttempts);
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    if (apiType == NatsApiType::Asynchronous) {
        natsOptions_SetMaxPendingMsgs(_opts, config.maxPendingMsgs);
        natsOptions_SetErrorHandler(_opts, ErrHandler, this);
    }
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

    // TODO, FIXME: Change so that connection happens NOT in the constructor.
    status = natsConnection_Connect(&_conn, _opts);
    if (status != NATS_OK) {
        throw PitayaException("unable to initialize nats server");
    }
}

NatsClientImpl::~NatsClientImpl()
{
    natsConnection_Destroy(_conn);
    natsOptions_Destroy(_opts);
}

NatsStatus
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
        return NatsStatus::Ok;
    }

    if (status == NATS_TIMEOUT) {
        return NatsStatus::Timeout;
    } else {
        return NatsStatus::UnknownErr;
    }
}

NatsClient::SubscriptionHandle
NatsClientImpl::Subscribe(const std::string& topic, std::function<void(std::shared_ptr<NatsMsg>)> onMessage)
{
    natsSubscription* sub;
    natsStatus status = natsConnection_Subscribe(&sub, _conn, topic.c_str(), HandleMsg, this);
    if (status != NATS_OK) {
        // TODO: log error here
        return NatsClient::kInvalidSubscriptionHandle;
    }
    
    SubscriptionHandler handler;
    handler.natsSubscription = sub;
    handler.onMessage = std::move(onMessage);

    _subscriptions.push_back(handler);
    return static_cast<void*>(sub);
}

void
NatsClientImpl::Unsubscribe(SubscriptionHandle handle)
{
    if (handle == NatsClient::kInvalidSubscriptionHandle) {
        throw PitayaException("Cannot unsubscribe invalid handle " +
                              std::to_string((size_t)handle));
    }

    auto it = std::find_if(
        _subscriptions.begin(), _subscriptions.end(), [=](const SubscriptionHandler& handler) {
            auto subscription = static_cast<natsSubscription*>(handle);
            return subscription == handler.natsSubscription;
        });

    if (it == _subscriptions.end()) {
        // If the handle was not found we throw an exception
        throw PitayaException("Cannot unsubscribe invalid handle " +
                              std::to_string((size_t)handle));
    }

    //
    // TODO: what to do here exactly?
    //
    natsSubscription_Drain(it->natsSubscription);
    natsSubscription_Unsubscribe(it->natsSubscription);
    natsSubscription_Destroy(it->natsSubscription);
    _subscriptions.erase(it);
}

NatsStatus
NatsClientImpl::Publish(const char* reply, const std::vector<uint8_t>& buf)
{
    natsStatus status = natsConnection_Publish(_conn, reply, buf.data(), buf.size());

    if (status != NATS_OK) {
        if (status == NATS_TIMEOUT) {
            return NatsStatus::Timeout;
        } else {
            return NatsStatus::UnknownErr;
        }
    }
    
    return NatsStatus::Ok;
}

void
NatsClientImpl::HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* user)
{
    auto natsClient = static_cast<NatsClientImpl*>(user);

    // Find the handler
    auto it = std::find_if(natsClient->_subscriptions.begin(),
                           natsClient->_subscriptions.end(),
                           [sub](const SubscriptionHandler& handler) {
                               return handler.natsSubscription == sub;
                           });
    

    if (it == natsClient->_subscriptions.end()) {
        // TODO: log error here
        return;
    }

    it->onMessage(std::shared_ptr<NatsMsg>(new NatsMsgImpl(msg)));
}

void
NatsClientImpl::DisconnectedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // TODO: implement logic here
    // instance->_log->error("nats disconnected! will try to reconnect...");
}

void
NatsClientImpl::ReconnectedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // TODO: implement logic here
    // instance->_log->error("nats reconnected!");
}

void
NatsClientImpl::ClosedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // TODO: implement logic here
    // instance->_log->error("failed all nats reconnection attempts!");
    // TODO: exit server here, but need to do this gracefully
}

void
NatsClientImpl::ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    if (err == NATS_SLOW_CONSUMER) {
        std::cerr << "nats runtime error: slow consumer\n";
    } else {
        std::cerr << "nats runtime error: " << err;
    }
}

} // namespace pitaya
