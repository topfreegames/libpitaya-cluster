#ifndef PITAYA_NATS_CLIENT_H
#define PITAYA_NATS_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats_config.h"

#include "spdlog/logger.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <nats/nats.h>
#include <string>
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
    SubscriptionErr,
};

enum class NatsApiType
{
    Synchronous,
    Asynchronous,
};

class NatsClient
{
public:
    virtual ~NatsClient() = default;

    virtual natsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                               const std::string& topic,
                               const std::vector<uint8_t>& data,
                               std::chrono::milliseconds timeout) = 0;

    virtual natsStatus Subscribe(const std::string& topic,
                                 std::function<void(std::shared_ptr<NatsMsg>)> onMessage) = 0;

    virtual natsStatus Publish(const char* reply, const std::vector<uint8_t>& buf) = 0;

    // Check if the client is currently in lame duck mode
    virtual bool IsInLameDuckMode() const = 0;

    // Check if we should buffer messages (during lame duck mode or reconnection)
    virtual bool ShouldBufferMessages() const = 0;
};

class NatsClientImpl : public NatsClient
{
public:
    NatsClientImpl(NatsApiType apiType, const NatsConfig& opts, const char* loggerName = nullptr);
    ~NatsClientImpl();

    natsStatus Request(std::shared_ptr<NatsMsg>* natsMsg,
                       const std::string& topic,
                       const std::vector<uint8_t>& data,
                       std::chrono::milliseconds timeout) override;

    natsStatus Subscribe(const std::string& topic,
                         std::function<void(std::shared_ptr<NatsMsg>)> onMessage) override;

    natsStatus Publish(const char* reply, const std::vector<uint8_t>& buf) override;

    // Check if the client is currently in lame duck mode
    bool IsInLameDuckMode() const override;

    // Check if we should buffer messages (during lame duck mode or reconnection)
    bool ShouldBufferMessages() const override;

private:
    static void DisconnectedCb(natsConnection* nc, void* user);
    static void ReconnectedCb(natsConnection* nc, void* user);
    static void ClosedCb(natsConnection* nc, void* user);
    static void LameDuckModeCb(natsConnection* nc, void* user);
    static void ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* user);
    static void HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* user);

private:
    struct SubscriptionHandler
    {
        std::function<void(std::shared_ptr<NatsMsg>)> onMessage;
    };

private:
    std::shared_ptr<spdlog::logger> _log;
    std::chrono::milliseconds _drainTimeout;
    std::chrono::milliseconds _lameDuckModeFlushTimeout;
    mutable std::mutex _lameDuckModeMutex;
    natsOptions* _opts;
    natsConnection* _conn;
    natsSubscription* _sub;
    std::function<void(std::shared_ptr<NatsMsg>)> _onMessage;
    bool _connClosed;
    bool _shuttingDown;
    bool _lameDuckMode;
};

} // namespace pitaya

#endif // PITAYA_NATS_CLIENT_H
