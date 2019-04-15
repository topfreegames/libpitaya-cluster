#ifndef PITAYA_NATS_CLIENT_H
#define PITAYA_NATS_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats_config.h"

#include <chrono>
#include <memory>
#include <nats/nats.h>
#include <unordered_map>
#include <vector>

namespace pitaya {

class NatsMsg
{
public:
    virtual ~NatsMsg() = default;
    virtual const uint8_t* GetData() const = 0;
    virtual size_t GetSize() const = 0;
    virtual const char* GetReply() const = 0;
};

class NatsMsgImpl : public NatsMsg
{
public:
    NatsMsgImpl(natsMsg* msg);
    ~NatsMsgImpl();

    const uint8_t* GetData() const override;
    size_t GetSize() const override;
    const char* GetReply() const override;

private:
    natsMsg* _msg;
};

enum class NatsStatus
{
    Ok = 0,
    Timeout,
    UnknownErr,
};

enum class NatsApiType
{
    Synchronous,
    Asynchronous,
};

class NatsClient
{
public:
    using SubscriptionHandle = void*;
    static constexpr SubscriptionHandle kInvalidSubscriptionHandle = nullptr;

    virtual ~NatsClient() = default;

    virtual NatsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                               const std::string& topic,
                               const std::vector<uint8_t>& data,
                               std::chrono::milliseconds timeout) = 0;

    virtual SubscriptionHandle Subscribe(
        const std::string& topic,
        std::function<void(std::shared_ptr<NatsMsg>)> onMessage) = 0;

    virtual void Unsubscribe(SubscriptionHandle handle) = 0;

    virtual NatsStatus Publish(const char* reply, const std::vector<uint8_t>& buf) = 0;
};

class NatsClientImpl : public NatsClient
{
public:
    NatsClientImpl(NatsApiType apiType, const NatsConfig& opts);
    ~NatsClientImpl();

    NatsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                       const std::string& topic,
                       const std::vector<uint8_t>& data,
                       std::chrono::milliseconds timeout) override;

    SubscriptionHandle Subscribe(const std::string& topic,
                                 std::function<void(std::shared_ptr<NatsMsg>)> onMessage) override;

    void Unsubscribe(SubscriptionHandle handle) override;

    NatsStatus Publish(const char* reply, const std::vector<uint8_t>& buf) override;

private:
    static void DisconnectedCb(natsConnection* nc, void* user);
    static void ReconnectedCb(natsConnection* nc, void* user);
    static void ClosedCb(natsConnection* nc, void* user);
    static void ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* user);
    static void HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* user);

private:
    struct SubscriptionHandler
    {
        natsSubscription* natsSubscription = nullptr;
        std::function<void(std::shared_ptr<NatsMsg>)> onMessage;
    };

    natsOptions* _opts;
    natsConnection* _conn;
    std::vector<SubscriptionHandler> _subscriptions;
};

} // namespace pitaya

#endif // PITAYA_NATS_CLIENT_H
