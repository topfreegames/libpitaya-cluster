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

//
// NatsClientImpl
//
NatsClientImpl::NatsClientImpl(const NatsConfig& config)
{
    natsStatus status = natsOptions_Create(&_opts);
    if (status != NATS_OK) {
        throw PitayaException("error configuring nats client");
    }

    natsOptions_SetTimeout(_opts, config.connectionTimeoutMs);
    natsOptions_SetMaxReconnect(_opts, config.maxReconnectionAttempts);
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

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

} // namespace pitaya
