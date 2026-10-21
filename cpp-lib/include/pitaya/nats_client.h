#ifndef PITAYA_NATS_CLIENT_H
#define PITAYA_NATS_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats_config.h"

#include "spdlog/logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <nats/nats.h>
#include <string>
#include <thread>
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

    // Check if the client is currently switching away from a server in lame duck mode
    virtual bool IsInLameDuckMode() const = 0;
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

    // Check if the client is currently switching away from a server in lame duck mode
    bool IsInLameDuckMode() const override;

private:
    // A NATS connection and the subscription on it. When a server enters lame duck mode,
    // the client opens a new connection, makes it the active one and drains the old one.
    struct Connection
    {
        natsConnection* conn = nullptr;
        natsSubscription* sub = nullptr;
        std::atomic<int> inFlight{ 0 };

        ~Connection();
    };

    static void DisconnectedCb(natsConnection* nc, void* user);
    static void ReconnectedCb(natsConnection* nc, void* user);
    static void ClosedCb(natsConnection* nc, void* user);
    static void LameDuckModeCb(natsConnection* nc, void* user);
    static void ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* user);
    static void HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* user);

    natsOptions* CreateOptions(NatsApiType apiType, bool retryOnFailedConnect);
    std::shared_ptr<Connection> AcquireActiveConnection();
    void SwitchConnection(std::thread previousSwitchThread);
    bool SwitchOnce();
    void DrainSubscription(Connection& connection);
    int64_t DrainTimeoutMs() const;
    int64_t FlushTimeoutMs() const;

private:
    std::shared_ptr<spdlog::logger> _log;
    NatsConfig _config;
    std::chrono::milliseconds _drainTimeout;
    std::chrono::milliseconds _lameDuckModeFlushTimeout;
    natsOptions* _opts;
    // Used for the connections opened during a lame duck switch: they fail fast instead of
    // retrying, so that a switch does not block while NATS is unreachable.
    natsOptions* _switchOpts;
    std::function<void(std::shared_ptr<NatsMsg>)> _onMessage;
    std::atomic<bool> _shuttingDown;

    // Guards _active and _topic.
    mutable std::mutex _activeMutex;
    std::shared_ptr<Connection> _active;
    std::string _topic;
    // Number of connections whose ClosedCb has not run yet.
    std::atomic<int> _openConnections;

    // Guards _switchThread and _pendingLameDuckConn. Taken before _activeMutex when both are
    // needed.
    std::mutex _switchMutex;
    std::atomic<bool> _switchInProgress;
    std::thread _switchThread;
    // A connection that entered lame duck mode while a switch was in progress.
    natsConnection* _pendingLameDuckConn;
};

} // namespace pitaya

#endif // PITAYA_NATS_CLIENT_H
