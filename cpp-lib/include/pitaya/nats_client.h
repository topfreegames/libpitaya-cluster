#ifndef PITAYA_NATS_CLIENT_H
#define PITAYA_NATS_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats_config.h"

#include "spdlog/logger.h"

#include <algorithm>
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

    // Check if the client is currently in lame duck mode
    virtual bool IsInLameDuckMode() const = 0;

    // Hot-swap support for zero-downtime lame duck mode
    virtual void SetHotSwapClient(std::shared_ptr<NatsClient> newClient) = 0;
    virtual std::shared_ptr<NatsClient> GetHotSwapClient() const = 0;
    virtual bool IsHotSwapAvailable() const = 0;
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

    // Hot-swap support for zero-downtime lame duck mode
    void SetHotSwapClient(std::shared_ptr<NatsClient> newClient) override;
    std::shared_ptr<NatsClient> GetHotSwapClient() const override;
    bool IsHotSwapAvailable() const override;

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

    // Request processing methods
    natsStatus ExecuteRequest(std::shared_ptr<NatsMsg>* msg,
                              const std::string& topic,
                              const std::vector<uint8_t>& data,
                              std::chrono::milliseconds timeout);

    natsStatus BufferRequest(std::shared_ptr<NatsMsg>* msg,
                             const std::string& topic,
                             const std::vector<uint8_t>& data,
                             std::chrono::milliseconds timeout);

    void ProcessPendingRequests();

private:
    struct SubscriptionHandler
    {
        std::function<void(std::shared_ptr<NatsMsg>)> onMessage;
    };

private:
    std::shared_ptr<spdlog::logger> _log;
    NatsConfig _config; // Store original configuration for hot-swap client creation
    std::chrono::milliseconds _drainTimeout;
    std::chrono::milliseconds _lameDuckModeFlushTimeout;
    mutable std::mutex _lameDuckModeMutex;
    natsOptions* _opts;
    natsConnection* _conn;
    natsSubscription* _sub;
    bool _connClosed;
    bool _shuttingDown;
    bool _lameDuckMode; // Lame duck mode flag
    std::function<void(std::shared_ptr<NatsMsg>)> _onMessage;

    // Hot-swap support for zero-downtime lame duck mode
    mutable std::mutex _hotSwapMutex;
    std::shared_ptr<NatsClient> _hotSwapClient;
    bool _hotSwapAvailable;

    // Application-level request buffering during lame duck mode
    struct PendingRequest
    {
        std::string topic;
        std::vector<uint8_t> data;
        std::chrono::milliseconds timeout;
        std::chrono::steady_clock::time_point timestamp;
        std::promise<std::pair<natsStatus, std::shared_ptr<NatsMsg>>> promise;
    };

    mutable std::mutex _pendingRequestsMutex;
    std::vector<std::unique_ptr<PendingRequest>> _pendingRequests;
    std::thread _requestProcessingThread;
    bool _processingPendingRequests;

    // Calculate max pending requests based on reconnectBufSize
    // Assumes average request size of ~1KB, so buffer can hold reconnectBufSize/1024 requests
    int GetMaxPendingRequests() const { return std::max(100, _config.reconnectBufSize / 1024); }
};

} // namespace pitaya

#endif // PITAYA_NATS_CLIENT_H
