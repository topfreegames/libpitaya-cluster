#ifndef PITAYA_NATS_CLIENT_H
#define PITAYA_NATS_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats/config.h"

#include <chrono>
#include <memory>
#include <nats/nats.h>
#include <vector>

namespace pitaya {

class NatsMsg
{
public:
    virtual ~NatsMsg() = default;
    virtual const uint8_t* GetData() const = 0;
    virtual size_t GetSize() const = 0;
};

class NatsMsgImpl : public NatsMsg
{
public:
    NatsMsgImpl(natsMsg* msg);
    ~NatsMsgImpl();

    const uint8_t* GetData() const override;
    size_t GetSize() const override;

private:
    natsMsg* _msg;
};

enum class NatsStatus
{
    Ok = 0,
    Timeout,
    UnknownErr,
};

class NatsClient
{
public:
    virtual ~NatsClient() = default;
    virtual bool Connect() = 0;
    virtual NatsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                               const std::string& topic,
                               const std::vector<uint8_t>& data,
                               std::chrono::milliseconds timeout) = 0;
};

class NatsClientImpl : public NatsClient
{
public:
    NatsClientImpl(const NatsConfig& opts);
    ~NatsClientImpl();

    bool Connect() override;
    NatsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                       const std::string& topic,
                       const std::vector<uint8_t>& data,
                       std::chrono::milliseconds timeout) override;

private:
    static void DisconnectedCb(natsConnection* nc, void* user);
    static void ReconnectedCb(natsConnection* nc, void* user);
    static void ClosedCb(natsConnection* nc, void* user);

private:
    natsOptions* _opts;
    natsConnection* _conn;
    std::chrono::milliseconds _requestTimeout;
};

} // namespace pitaya

#endif // PITAYA_NATS_CLIENT_H