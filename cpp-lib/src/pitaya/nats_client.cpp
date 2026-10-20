#include "pitaya/nats_client.h"

#include "pitaya/utils.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string>

// #region agent log
static std::mutex g_debugLogMutex;
static void
debugLog(const std::string& location,
         const std::string& message,
         const std::string& hypothesisId,
         const std::string& data = "{}")
{
    std::lock_guard<std::mutex> lock(g_debugLogMutex);
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::cout << "[DEBUG_AGENT] {\"location\":\"" << location << "\",\"message\":\"" << message
              << "\",\"hypothesisId\":\"" << hypothesisId << "\",\"timestamp\":" << ms
              << ",\"data\":" << data << "}" << std::endl;
}
// #endregion

namespace pitaya {

//
// NatsMsgImpl
//
NatsMsgImpl::NatsMsgImpl(natsMsg* msg)
    : _msg(msg)
{
}

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
static constexpr std::chrono::milliseconds kSubscriptionPropagationDelay(1000);
static constexpr std::chrono::milliseconds kRestoreSubscriptionRetryDelay(1000);

//
// NatsClientImpl
//
NatsClientImpl::NatsClientImpl(NatsApiType apiType,
                               const NatsConfig& config,
                               const char* loggerName,
                               bool isHotSwap)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _config(config)
    , _apiType(apiType)
    , _opts(nullptr)
    , _conn(nullptr)
    , _sub(nullptr)
    , _connClosed(false)
    , _shuttingDown(false)
    , _lameDuckMode(false)
    , _isHotSwap(isHotSwap)
    , _inFlightRequests(0)
    , _lameDuckSwitchState(LameDuckSwitchState::Idle)
    , _lameDuckSwitchGeneration(0)
    , _restoreSubscriptionGeneration(0)
    , _onMessage(nullptr)
    , _hotSwapAvailable(false)
    , _processingPendingRequests(false)
    , _drainTimeout(config.drainTimeout)
    , _lameDuckModeFlushTimeout(config.flushTimeout)
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

    // Reconnect parameters
    natsOptions_SetReconnectBufSize(_opts, config.reconnectBufSize);
    natsOptions_SetReconnectWait(_opts, config.reconnectWaitInMs.count());
    // Only enable RetryOnFailedConnect if maxReconnectionAttempts > 0
    // Otherwise the initial connection will hang indefinitely
    // A hot-swap client fails fast, so that the lame duck switch and the destructor that
    // waits for it do not block while NATS is unreachable.
    bool retryOnFailedConnect = !_isHotSwap && (config.maxReconnectionAttempts > 0);
    natsOptions_SetRetryOnFailedConnect(_opts, retryOnFailedConnect, NULL, this);
    natsOptions_SetPingInterval(_opts, config.pingIntervalInMs.count());
    natsOptions_SetMaxPingsOut(_opts, config.maxPingsOut);
    natsOptions_SetReconnectJitter(
        _opts, config.reconnectJitterInMs.count(), config.reconnectJitterInMs.count());

    // Callbacks
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    // Register lame duck mode handler
    if (!_isHotSwap) {
        natsOptions_SetLameDuckModeCB(_opts, LameDuckModeCb, this);
    }

    if (apiType == NatsApiType::Asynchronous) {
        natsOptions_SetMaxPendingMsgs(_opts, config.maxPendingMsgs);
        natsOptions_SetErrorHandler(_opts, ErrHandler, this);
    }
    natsOptions_SetURL(_opts, config.natsAddr.c_str());

    _log->info("NATS Connection Timeout - " + std::to_string(config.connectionTimeout.count()));
    _log->info("NATS Max Reconnect Attempts - " + std::to_string(config.maxReconnectionAttempts));
    _log->info("NATS Reconnect Wait Time - " + std::to_string(config.reconnectWaitInMs.count()));
    _log->info("NATS Reconnect Buffer Size - " + std::to_string(config.reconnectBufSize));
    _log->info("NATS Ping Interval - " + std::to_string(config.pingIntervalInMs.count()));
    _log->info("NATS Max Pings Out - " + std::to_string(config.maxPingsOut));
    _log->info("NATS Reconnect Jitter - " + std::to_string(config.reconnectJitterInMs.count()));

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
    _shuttingDown = true;

    std::thread lameDuckSwitchThread;
    std::thread restoreSubscriptionThread;
    {
        std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
        lameDuckSwitchThread = std::move(_lameDuckSwitchThread);
        restoreSubscriptionThread = std::move(_restoreSubscriptionThread);
    }
    if (lameDuckSwitchThread.joinable()) {
        lameDuckSwitchThread.join();
    }
    if (restoreSubscriptionThread.joinable()) {
        restoreSubscriptionThread.join();
    }

    // Drain the hot-swap subscriptions before the primary one. A completed drain also means
    // that no message callback is still running. Only the threads joined above retire
    // hot-swap clients, so no new one can be added after this point.
    std::shared_ptr<NatsClient> hotSwapClient;
    {
        std::lock_guard<std::mutex> lock(_hotSwapMutex);
        hotSwapClient = std::move(_hotSwapClient);
        _hotSwapAvailable = false;
    }
    if (auto hotSwapClientImpl = std::dynamic_pointer_cast<NatsClientImpl>(hotSwapClient)) {
        hotSwapClientImpl->DrainSubscription();
    }
    hotSwapClient.reset();

    std::vector<RetiringHotSwapClient> retiringHotSwapClients;
    {
        std::lock_guard<std::mutex> lock(_retiringHotSwapClientsMutex);
        retiringHotSwapClients.swap(_retiringHotSwapClients);
    }
    for (auto& retiring : retiringHotSwapClients) {
        retiring.thread.join();
    }

    // Stop request processing thread
    {
        std::lock_guard<std::mutex> lock(_pendingRequestsMutex);
        _processingPendingRequests = false;
    }

    // Wait for request processing thread to finish
    if (_requestProcessingThread.joinable()) {
        _requestProcessingThread.join();
    }
    DrainSubscription();

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
    // Check if in lame duck mode
    if (IsInLameDuckMode()) {
        // Try to use hot-swap client first
        auto hotSwapClient = GetHotSwapClient();
        if (hotSwapClient && IsHotSwapAvailable()) {
            _log->info("Using hot-swap client for request during lame duck mode");
            return hotSwapClient->Request(msg, topic, data, timeout);
        }

        // If no hot-swap client available, buffer the request
        _log->info("Buffering request during lame duck mode - topic: {}", topic);
        return BufferRequest(msg, topic, data, timeout);
    }

    return ExecuteRequest(msg, topic, data, timeout);
}

natsStatus
NatsClientImpl::ExecuteRequest(std::shared_ptr<NatsMsg>* msg,
                               const std::string& topic,
                               const std::vector<uint8_t>& data,
                               std::chrono::milliseconds timeout)
{
    natsMsg* reply = nullptr;
    ++_inFlightRequests;
    natsStatus status = natsConnection_Request(
        &reply, _conn, topic.c_str(), data.data(), data.size(), timeout.count());
    --_inFlightRequests;

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
NatsClientImpl::BufferRequest(std::shared_ptr<NatsMsg>* msg,
                              const std::string& topic,
                              const std::vector<uint8_t>& data,
                              std::chrono::milliseconds timeout)
{
    auto request = std::make_unique<PendingRequest>();
    request->topic = topic;
    request->data = data;
    request->timeout = timeout;
    request->timestamp = std::chrono::steady_clock::now();

    auto future = request->promise.get_future();

    {
        std::lock_guard<std::mutex> lock(_pendingRequestsMutex);

        // Check if we're approaching the buffer limit
        int maxPendingRequests = GetMaxPendingRequests();
        if (_pendingRequests.size() >= maxPendingRequests) {
            _log->warn("Application request buffer full ({} requests) - dropping new request for "
                       "topic: {}",
                       _pendingRequests.size(),
                       topic);
            return NATS_INSUFFICIENT_BUFFER;
        }

        _pendingRequests.push_back(std::move(request));
        _log->debug("Buffered request for topic: {} (buffer: {}/{})",
                    topic,
                    _pendingRequests.size(),
                    maxPendingRequests);

        // Start processing thread if not already running
        if (!_processingPendingRequests) {
            _processingPendingRequests = true;
            if (_requestProcessingThread.joinable()) {
                _requestProcessingThread.join();
            }
            _requestProcessingThread = std::thread(&NatsClientImpl::ProcessPendingRequests, this);
        }
    }

    // Wait for the request to be processed
    try {
        auto futureStatus = future.wait_for(timeout + std::chrono::seconds(5)); // Add 5s buffer
        if (futureStatus == std::future_status::timeout) {
            _log->warn("Buffered request timed out for topic: {}", topic);
            return NATS_TIMEOUT;
        }

        auto result = future.get();
        *msg = result.second;
        return result.first;
    } catch (const std::exception& e) {
        _log->error("Error processing buffered request: {}", e.what());
        return NATS_ERR;
    }
}

natsStatus
NatsClientImpl::Subscribe(const std::string& topic,
                          std::function<void(std::shared_ptr<NatsMsg>)> onMessage)
{
    // Check if in lame duck mode
    if (IsInLameDuckMode()) {
        _log->warn("Attempting to subscribe during lame duck mode - operation skipped");
        return NATS_ILLEGAL_STATE;
    }

    _log->info("Subscribing to topic {}", topic);
    _onMessage = std::move(onMessage);
    {
        std::lock_guard<std::mutex> lock(_subMutex);
        _topic = topic;
    }
    return QueueSubscribe();
}

void
NatsClientImpl::DrainSubscription()
{
    natsSubscription* sub = nullptr;
    {
        // Clearing the topic keeps ReconnectedCb from subscribing again.
        std::lock_guard<std::mutex> lock(_subMutex);
        std::swap(sub, _sub);
        _topic.clear();
    }
    if (!sub) {
        return;
    }

    natsStatus status = natsSubscription_Drain(sub);
    if (status == NATS_OK) {
        status = natsSubscription_WaitForDrainCompletion(sub, DrainTimeoutMs());
    }
    if (status != NATS_OK) {
        int pendingMsgs = 0;
        natsSubscription_GetPending(sub, &pendingMsgs, nullptr);
        _log->error("Subscription drain did not complete ({}), dropping {} messages",
                    natsStatus_GetText(status),
                    pendingMsgs);
    }
    // Destroy unsubscribes the subscription if the drain did not complete.
    natsSubscription_Destroy(sub);
}

std::string
NatsClientImpl::ConnectedServerId() const
{
    char serverId[256] = { 0 };
    if (natsConnection_GetConnectedServerId(_conn, serverId, sizeof(serverId)) != NATS_OK) {
        return std::string();
    }
    return std::string(serverId);
}

int64_t
NatsClientImpl::DrainTimeoutMs() const
{
    // natsSubscription_WaitForDrainCompletion waits forever for a timeout that is not positive.
    return _drainTimeout.count() > 0 ? _drainTimeout.count()
                                     : PITAYA_NATS_DEFAULT_DRAIN_TIMEOUT_IN_MS;
}

int64_t
NatsClientImpl::FlushTimeoutMs() const
{
    // natsConnection_FlushTimeout rejects a timeout that is not positive.
    return _lameDuckModeFlushTimeout.count() > 0 ? _lameDuckModeFlushTimeout.count()
                                                 : PITAYA_NATS_DEFAULT_FLUSH_TIMEOUT_IN_MS;
}

natsStatus
NatsClientImpl::QueueSubscribe()
{
    // The topic is also the queue name, so that the subscriptions of the primary and the
    // hot-swap clients can coexist during lame duck mode without delivering a message twice.
    std::lock_guard<std::mutex> lock(_subMutex);
    if (_shuttingDown) {
        return NATS_ILLEGAL_STATE;
    }
    if (_sub || _topic.empty()) {
        return NATS_OK;
    }

    natsStatus status = natsConnection_QueueSubscribe(
        &_sub, _conn, _topic.c_str(), _topic.c_str(), HandleMsg, this);
    if (status != NATS_OK) {
        _sub = nullptr;
        _log->error("Failed to subscribe: {}", natsStatus_GetText(status));
    }
    return status;
}

// Hot-swap support for zero-downtime lame duck mode
void
NatsClientImpl::SetHotSwapClient(std::shared_ptr<NatsClient> newClient)
{
    // #region agent log
    std::string instanceAddr = std::to_string(reinterpret_cast<uintptr_t>(this));
    debugLog("nats_client.cpp:SetHotSwapClient",
             "BEFORE acquiring hotSwapMutex",
             "A",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion
    std::lock_guard<std::mutex> lock(_hotSwapMutex);
    // #region agent log
    debugLog("nats_client.cpp:SetHotSwapClient",
             "ACQUIRED hotSwapMutex",
             "A",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion
    _hotSwapClient = newClient;
    _hotSwapAvailable = (newClient != nullptr);
    if (_hotSwapAvailable) {
        _log->info("Hot-swap client set - zero-downtime lame duck mode enabled");
    }
    // #region agent log
    debugLog("nats_client.cpp:SetHotSwapClient",
             "RELEASING hotSwapMutex",
             "A",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion
}

std::shared_ptr<NatsClient>
NatsClientImpl::GetHotSwapClient() const
{
    std::lock_guard<std::mutex> lock(_hotSwapMutex);
    return _hotSwapClient;
}

bool
NatsClientImpl::IsHotSwapAvailable() const
{
    std::lock_guard<std::mutex> lock(_hotSwapMutex);
    return _hotSwapAvailable;
}

bool
NatsClientImpl::IsInLameDuckMode() const
{
    std::lock_guard<std::mutex> lock(_lameDuckModeMutex);
    return _lameDuckMode;
}

natsStatus
NatsClientImpl::Publish(const char* reply, const std::vector<uint8_t>& buf)
{
    // During lame duck mode, publish through the hot-swap client when it is available.
    // Otherwise the message is buffered by NATS.c while the connection reconnects.
    if (IsInLameDuckMode()) {
        auto hotSwapClient = GetHotSwapClient();
        if (hotSwapClient) {
            return hotSwapClient->Publish(reply, buf);
        }
        _log->info("Publishing during lame duck mode - message will be buffered for reconnection");
    }

    natsStatus status = natsConnection_Publish(_conn, reply, buf.data(), buf.size());

    if (status != NATS_OK) {
        switch (status) {
            case NATS_TIMEOUT:
                _log->warn("NATS publish timeout");
                return status;
            case NATS_INSUFFICIENT_BUFFER:
                _log->warn("NATS reconnection buffer full during lame duck mode - message dropped");
                return status;
            default:
                _log->error("NATS publish failed: {}", natsStatus_GetText(status));
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
    // #region agent log
    std::string instanceAddr = std::to_string(reinterpret_cast<uintptr_t>(instance));
    debugLog("nats_client.cpp:ReconnectedCb",
             "Reconnection callback received",
             "C",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion
    instance->_log->warn("nats reconnected!");

    // Reset lame duck mode flag after successful reconnection
    {
        std::lock_guard<std::mutex> lock(instance->_lameDuckModeMutex);
        if (instance->_lameDuckMode) {
            instance->_lameDuckMode = false;
            instance->_log->info("Lame duck mode flag reset after successful reconnection");
        }
    }

    // The subscription is restored on a separate thread, because this callback runs on the
    // NATS thread that serves the callbacks of all connections.
    std::lock_guard<std::mutex> lock(instance->_lameDuckSwitchMutex);
    instance->_lameDuckSwitchState = LameDuckSwitchState::Idle;
    if (instance->_shuttingDown) {
        return;
    }
    std::thread previousRestoreThread = std::move(instance->_restoreSubscriptionThread);
    instance->_restoreSubscriptionThread =
        std::thread(&NatsClientImpl::RestoreSubscription,
                    instance,
                    ++instance->_restoreSubscriptionGeneration,
                    std::move(previousRestoreThread));
}

void
NatsClientImpl::RestoreSubscription(uint64_t generation, std::thread previousRestoreThread)
{
    if (previousRestoreThread.joinable()) {
        previousRestoreThread.join();
    }

    for (;;) {
        {
            std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
            if (_shuttingDown || generation != _restoreSubscriptionGeneration) {
                return;
            }
        }

        std::string topic;
        bool needsSubscription;
        {
            std::lock_guard<std::mutex> lock(_subMutex);
            topic = _topic;
            needsSubscription = !topic.empty() && _sub == nullptr;
        }

        // The hot-swap client is released only when the primary subscription is confirmed
        // by the server. Until then the hot-swap subscription keeps receiving the messages.
        natsStatus status = NATS_OK;
        if (!topic.empty()) {
            if (needsSubscription) {
                status = QueueSubscribe();
            }
            if (status == NATS_OK) {
                status = natsConnection_FlushTimeout(_conn, FlushTimeoutMs());
            }
        }

        if (status == NATS_OK) {
            if (needsSubscription) {
                _log->info("Subscription to {} restored after reconnection", topic);
            }
            std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
            // A new lame duck switch owns the hot-swap client from now on.
            if (!_shuttingDown && generation == _restoreSubscriptionGeneration &&
                _lameDuckSwitchState == LameDuckSwitchState::Idle) {
                ReleaseHotSwapClient();
            }
            return;
        }

        _log->error("Failed to confirm subscription to {} after reconnection ({}) - retrying",
                    topic,
                    natsStatus_GetText(status));
        auto retryAt = std::chrono::steady_clock::now() + kRestoreSubscriptionRetryDelay;
        while (!_shuttingDown && std::chrono::steady_clock::now() < retryAt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void
NatsClientImpl::ReleaseHotSwapClient()
{
    std::shared_ptr<NatsClient> staleHotSwapClient;
    {
        std::lock_guard<std::mutex> lock(_hotSwapMutex);
        staleHotSwapClient = std::move(_hotSwapClient);
        _hotSwapAvailable = false;
    }
    if (!staleHotSwapClient) {
        return;
    }

    _log->info("Releasing hot-swap client after reconnection");
    // The delay lets the restored primary subscription reach the whole cluster before the
    // hot-swap subscription is drained. The destructor joins this thread, so it can use
    // this instance.
    auto done = std::make_shared<std::atomic<bool>>(false);
    std::thread thread([this, client = std::move(staleHotSwapClient), done]() mutable {
        auto drainAt = std::chrono::steady_clock::now() + kSubscriptionPropagationDelay;
        while (!_shuttingDown && std::chrono::steady_clock::now() < drainAt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (auto hotSwapClient = std::dynamic_pointer_cast<NatsClientImpl>(client)) {
            hotSwapClient->DrainSubscription();
        }
        client.reset();
        *done = true;
    });

    std::lock_guard<std::mutex> lock(_retiringHotSwapClientsMutex);
    for (auto it = _retiringHotSwapClients.begin(); it != _retiringHotSwapClients.end();) {
        if (*it->done) {
            it->thread.join();
            it = _retiringHotSwapClients.erase(it);
        } else {
            ++it;
        }
    }
    _retiringHotSwapClients.push_back({ std::move(thread), std::move(done) });
}

void
NatsClientImpl::ClosedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // #region agent log
    std::string instanceAddr = std::to_string(reinterpret_cast<uintptr_t>(instance));
    debugLog("nats_client.cpp:ClosedCb",
             "Connection closed callback - CRITICAL",
             "D",
             "{\"instance\":\"" + instanceAddr + "\",\"shuttingDown\":" +
                 std::string(instance->_shuttingDown ? "true" : "false") + "}");
    // #endregion
    // Signal main thread that the connection was actually closed
    instance->_log->warn("nats connection closed!");
    instance->_connClosed = true;
    if (!instance->_shuttingDown && !instance->_isHotSwap) {
        // #region agent log
        debugLog("nats_client.cpp:ClosedCb",
                 "SENDING SIGTERM - all reconnection attempts failed",
                 "D",
                 "{\"instance\":\"" + instanceAddr + "\"}");
        // #endregion
        // This was not initiated by a shutdown request.
        // Send SIGTERM as this was caused by all reconnection attempts failing
        std::thread(std::bind(raise, SIGTERM)).detach();
    }
}

void
NatsClientImpl::ProcessPendingRequests()
{
    _log->info("Started pending requests processing thread");

    while (_processingPendingRequests && !_shuttingDown) {
        std::vector<std::unique_ptr<PendingRequest>> currentRequests;

        // Get current pending requests
        {
            std::lock_guard<std::mutex> lock(_pendingRequestsMutex);
            if (_pendingRequests.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            currentRequests.swap(_pendingRequests);
        }

        // Process each request
        for (auto& request : currentRequests) {
            auto elapsed = std::chrono::steady_clock::now() - request->timestamp;
            if (elapsed > request->timeout) {
                // Request has timed out
                request->promise.set_value({ NATS_TIMEOUT, nullptr });
                continue;
            }

            // Check if hot-swap client is available or if lame duck mode has ended
            std::shared_ptr<NatsClient> clientToUse = nullptr;
            bool usePrimaryClient = false;

            // #region agent log
            debugLog("nats_client.cpp:ProcessPendingRequests",
                     "BEFORE acquiring locks atomically (FIX: using std::lock to prevent deadlock)",
                     "A",
                     "{\"thread\":\"processing\"}");
            // #endregion
            {
                // FIX: Use std::lock() to acquire both locks atomically, preventing ABBA deadlock
                // Previously: acquired hotSwapMutex then lameDuckModeMutex (could deadlock with
                // LameDuckModeCb) Now: acquire both atomically using std::lock()
                std::unique_lock<std::mutex> hotSwapLock(_hotSwapMutex, std::defer_lock);
                std::unique_lock<std::mutex> lameDuckLock(_lameDuckModeMutex, std::defer_lock);
                std::lock(hotSwapLock, lameDuckLock);
                // #region agent log
                debugLog("nats_client.cpp:ProcessPendingRequests",
                         "ACQUIRED both locks atomically (deadlock-free)",
                         "A",
                         "{\"thread\":\"processing\"}");
                // #endregion

                if (_hotSwapAvailable && _hotSwapClient) {
                    clientToUse = _hotSwapClient;
                    _log->info("Using hot-swap client for buffered request");
                } else if (!_lameDuckMode) {
                    usePrimaryClient = true;
                    _log->info("Using primary client for buffered request (lame duck mode ended)");
                }
            }

            if (clientToUse) {
                // Use hot-swap client
                std::shared_ptr<NatsMsg> msg;
                natsStatus status =
                    clientToUse->Request(&msg, request->topic, request->data, request->timeout);
                request->promise.set_value({ status, msg });
            } else if (usePrimaryClient) {
                // Use primary client (lame duck mode has ended)
                std::shared_ptr<NatsMsg> msg;
                natsStatus status =
                    ExecuteRequest(&msg, request->topic, request->data, request->timeout);
                request->promise.set_value({ status, msg });
            } else {
                // Re-queue the request if neither client is available
                std::lock_guard<std::mutex> lock(_pendingRequestsMutex);
                _pendingRequests.push_back(std::move(request));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Process any remaining requests with error
    {
        std::lock_guard<std::mutex> lock(_pendingRequestsMutex);
        for (auto& request : _pendingRequests) {
            request->promise.set_value({ NATS_ERR, nullptr });
        }
        _pendingRequests.clear();
    }

    _log->info("Stopped pending requests processing thread");
}

void
NatsClientImpl::LameDuckModeCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);

    // #region agent log
    std::string instanceAddr = std::to_string(reinterpret_cast<uintptr_t>(instance));
    debugLog("nats_client.cpp:LameDuckModeCb:ENTRY",
             "Lame duck callback entered",
             "B",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion

    instance->_log->info("=== LAME DUCK MODE DETECTED ===");
    char serverUrl[256];
    natsConnection_GetConnectedUrl(nc, serverUrl, sizeof(serverUrl));
    instance->_log->debug("Server: {}", serverUrl);

    {
        std::lock_guard<std::mutex> lock(instance->_lameDuckModeMutex);
        instance->_lameDuckMode = true;
        instance->_log->debug("Lame duck mode flag set - preventing new operations");
    }

    // An empty server ID means that the connection is no longer connected to the server in
    // lame duck mode, so the reconnection is already in progress.
    std::string lameDuckServerId = instance->ConnectedServerId();
    if (lameDuckServerId.empty()) {
        instance->_log->info("Connection already left the lame duck server - no switch needed");
        return;
    }

    // The switch runs on a separate thread, because this callback runs on the NATS thread
    // that serves the callbacks of all connections.
    std::lock_guard<std::mutex> lock(instance->_lameDuckSwitchMutex);
    if (instance->_shuttingDown || instance->_lameDuckSwitchState != LameDuckSwitchState::Idle) {
        instance->_log->info("Lame duck switch already in progress - ignoring callback");
        return;
    }
    instance->_lameDuckSwitchState = LameDuckSwitchState::Switching;
    std::thread previousSwitchThread = std::move(instance->_lameDuckSwitchThread);
    instance->_lameDuckSwitchThread = std::thread(&NatsClientImpl::SwitchAwayFromLameDuckServer,
                                                  instance,
                                                  nc,
                                                  ++instance->_lameDuckSwitchGeneration,
                                                  std::move(lameDuckServerId),
                                                  std::move(previousSwitchThread));
}

void
NatsClientImpl::SwitchAwayFromLameDuckServer(natsConnection* nc,
                                             uint64_t generation,
                                             std::string lameDuckServerId,
                                             std::thread previousSwitchThread)
{
    // A previous switch that was stopped by a reconnection can still be finishing.
    if (previousSwitchThread.joinable()) {
        previousSwitchThread.join();
    }

    // #region agent log
    std::string instanceAddr = std::to_string(reinterpret_cast<uintptr_t>(this));
    debugLog("nats_client.cpp:DrainThread:ENTRY",
             "Drain thread started",
             "D",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion

    // Must be called with _lameDuckSwitchMutex held. A reconnection resets the state in
    // ReconnectedCb, but that callback can be queued behind other callbacks. The server ID
    // shows a reconnection that ReconnectedCb has not reported yet.
    auto isCurrentSwitch = [this, generation, &lameDuckServerId]() {
        return !_shuttingDown && _lameDuckSwitchGeneration == generation &&
               _lameDuckSwitchState == LameDuckSwitchState::Switching &&
               ConnectedServerId() == lameDuckServerId;
    };

    // 1. Create the hot-swap client and subscribe it, so that it receives the messages
    // while the primary subscription is drained and the primary connection reconnects.
    if (!IsHotSwapAvailable()) {
        _log->debug("Creating hot-swap client for zero-downtime failover...");
        std::shared_ptr<NatsClientImpl> hotSwapClient;
        try {
            std::string hotSwapLoggerName = std::string(_log->name()) + "_hotswap";
            hotSwapClient = std::make_shared<NatsClientImpl>(
                _apiType, _config, hotSwapLoggerName.c_str(), true);
        } catch (const std::exception& e) {
            _log->error("Failed to create hot-swap client: {}", e.what());
        }
        if (hotSwapClient) {
            std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
            if (isCurrentSwitch()) {
                SetHotSwapClient(hotSwapClient);
            }
        }
        // If the switch already stopped, the hot-swap client is destroyed here.
    }

    bool hotSwapSubscribed = false;
    std::string topic;
    {
        std::lock_guard<std::mutex> lock(_subMutex);
        topic = _topic;
    }
    if (!topic.empty()) {
        auto hotSwapClient = std::dynamic_pointer_cast<NatsClientImpl>(GetHotSwapClient());
        if (hotSwapClient) {
            bool alreadySubscribed;
            {
                std::lock_guard<std::mutex> lock(hotSwapClient->_subMutex);
                alreadySubscribed = hotSwapClient->_sub != nullptr;
            }
            natsStatus status =
                alreadySubscribed ? NATS_OK : hotSwapClient->Subscribe(topic, _onMessage);
            if (status == NATS_OK) {
                status = natsConnection_FlushTimeout(hotSwapClient->_conn,
                                                     hotSwapClient->FlushTimeoutMs());
            }
            if (status == NATS_OK) {
                hotSwapSubscribed = true;
                _log->info("Hot-swap client subscribed to {}", topic);
            } else {
                _log->warn("Failed to subscribe hot-swap client: {}", natsStatus_GetText(status));
            }
        }
    }
    if (hotSwapSubscribed) {
        std::this_thread::sleep_for(kSubscriptionPropagationDelay);
    }

    // 2. Drain the primary subscription only when the hot-swap subscription is active.
    // Otherwise keep it, so that NATS.c sends it again to the new server.
    natsSubscription* drainingSub = nullptr;
    {
        std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
        if (!isCurrentSwitch()) {
            _log->info("Lame duck switch stopped: connection already reconnected");
            return;
        }
        if (hotSwapSubscribed) {
            std::lock_guard<std::mutex> subLock(_subMutex);
            std::swap(drainingSub, _sub);
        }
    }
    // The drain and the requests in flight share one deadline, so that the switch ends
    // well before the server closes the connection.
    auto deadline = std::chrono::steady_clock::now() +
                    std::max(std::chrono::milliseconds(DrainTimeoutMs()), _config.requestTimeout);
    if (drainingSub) {
        _log->debug("Draining primary subscription...");
        natsStatus status = natsSubscription_Drain(drainingSub);
        if (status == NATS_OK) {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            status = natsSubscription_WaitForDrainCompletion(
                drainingSub, std::max<int64_t>(remaining.count(), 1));
        }
        if (status != NATS_OK) {
            int pendingMsgs = 0;
            natsSubscription_GetPending(drainingSub, &pendingMsgs, nullptr);
            _log->error("Primary subscription drain did not complete ({}), dropping {} messages",
                        natsStatus_GetText(status),
                        pendingMsgs);
        }
        // Destroy unsubscribes the subscription if the drain did not complete.
        natsSubscription_Destroy(drainingSub);
    }

    // 3. Wait for the requests sent on the primary connection, so that their replies are
    // not lost. New requests use the hot-swap client or are buffered.
    while (_inFlightRequests > 0 && !_shuttingDown &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (_inFlightRequests > 0) {
        _log->warn("Reconnecting with {} requests still in flight", _inFlightRequests.load());
    }

    // 4. Flush pending messages to current server
    _log->debug("Flushing pending messages...");
    natsStatus status = natsConnection_FlushTimeout(nc, FlushTimeoutMs());
    // #region agent log
    debugLog("nats_client.cpp:DrainThread",
             "FlushTimeout returned",
             "D",
             "{\"status\":\"" + std::string(natsStatus_GetText(status)) + "\",\"instance\":\"" +
                 instanceAddr + "\"}");
    // #endregion
    if (status == NATS_OK) {
        _log->debug("Successfully flushed pending messages");
    } else {
        _log->warn("Flush failed: {}", natsStatus_GetText(status));
    }

    // 5. Trigger reconnection to other servers. ReconnectedCb restores the primary
    // subscription and releases the hot-swap client. natsConnection_Reconnect only shuts
    // down the socket, so it is called under the lock, right after the check.
    {
        std::lock_guard<std::mutex> lock(_lameDuckSwitchMutex);
        if (!isCurrentSwitch()) {
            _log->info("Lame duck switch stopped: connection already reconnected");
            return;
        }
        _lameDuckSwitchState = LameDuckSwitchState::Reconnecting;
        _log->debug("Initiating reconnection...");
        natsConnection_Reconnect(nc);
    }

    _log->info("=== LAME DUCK MODE HANDLING COMPLETE ===");
    // #region agent log
    debugLog("nats_client.cpp:DrainThread:EXIT",
             "Drain thread completed",
             "D",
             "{\"instance\":\"" + instanceAddr + "\"}");
    // #endregion
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
