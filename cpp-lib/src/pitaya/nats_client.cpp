#include "pitaya/nats_client.h"

#include "pitaya/utils.h"

#include <signal.h>
#include <string>

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
    , _shuttingDown(false)
    , _lameDuckMode(false)
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
    natsOptions_SetRetryOnFailedConnect(_opts, true, NULL, this);
    natsOptions_SetPingInterval(_opts, config.pingIntervalInMs.count());
    natsOptions_SetMaxPingsOut(_opts, config.maxPingsOut);
    natsOptions_SetReconnectJitter(
        _opts, config.reconnectJitterInMs.count(), config.reconnectJitterInMs.count());

    // Callbacks
    natsOptions_SetClosedCB(_opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(_opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(_opts, ReconnectedCb, this);
    // Register lame duck mode handler
    natsOptions_SetLameDuckModeCB(_opts, LameDuckModeCb, this);

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
    if (_sub) {
        // Remove interest from the subscription. Note that pending message may still
        // be received by the client.
        natsStatus status;
        _log->debug("Unsubscribing from NATS subscription");
        status = natsSubscription_Unsubscribe(_sub);
        if (status != NATS_OK) {
            _log->error("Failed to unsubscribe");
        }

        _log->debug("Draining NATS subscription");
        status = natsSubscription_Drain(_sub);
        if (status != NATS_OK) {
            _log->error("Failed to drain subscription");
        }

        status = natsSubscription_WaitForDrainCompletion(_sub, _drainTimeout.count());
        if (status != NATS_OK) {
            _log->error("Failed to wait for subscription drain");
        }
        // Called only here, because it needs to wait for the
        // natsSubscription_WaitForDrainCompletion
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
    // Check if in lame duck mode
    if (IsInLameDuckMode()) {
        _log->warn("Attempting to make request during lame duck mode - operation skipped");
        return NATS_ILLEGAL_STATE;
    }

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
    // Check if in lame duck mode
    if (IsInLameDuckMode()) {
        _log->warn("Attempting to subscribe during lame duck mode - operation skipped");
        return NATS_ILLEGAL_STATE;
    }

    _log->info("Subscribing to topic {}", topic);
    _onMessage = std::move(onMessage);
    natsStatus status = natsConnection_Subscribe(&_sub, _conn, topic.c_str(), HandleMsg, this);
    if (status != NATS_OK) {
        _log->error("Failed to subscribe");
        return status;
    }
    return status;
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
    // During lame duck mode, allow publishing but it will be buffered by NATS.c
    // This is different from other operations which are blocked
    {
        std::lock_guard<std::mutex> lock(_lameDuckModeMutex);
        if (_lameDuckMode) {
            _log->info(
                "Publishing during lame duck mode - message will be buffered for reconnection");
        }
    }

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
    instance->_log->warn("nats reconnected!");

    // Reset lame duck mode flag after successful reconnection
    {
        std::lock_guard<std::mutex> lock(instance->_lameDuckModeMutex);
        if (instance->_lameDuckMode) {
            instance->_lameDuckMode = false;
            instance->_log->info("Lame duck mode flag reset after successful reconnection");
        }
    }
}

void
NatsClientImpl::ClosedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // Signal main thread that the connection was actually closed
    instance->_log->info("nats connection closed!");
    instance->_connClosed = true;
    if (!instance->_shuttingDown) {
        // This was not initiated by a shutdown request.
        // Send SIGTERM as this was caused by all reconnection attempts failing
        std::thread(std::bind(raise, SIGTERM)).detach();
    }
}

void
NatsClientImpl::LameDuckModeCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);

    instance->_log->info("=== LAME DUCK MODE DETECTED ===");
    char serverUrl[256];
    natsConnection_GetConnectedUrl(nc, serverUrl, sizeof(serverUrl));
    instance->_log->info("Server: {}", serverUrl);

    {
        std::lock_guard<std::mutex> lock(instance->_lameDuckModeMutex);
        instance->_lameDuckMode = true;
        instance->_log->info("Lame duck mode flag set - preventing new operations");
    }

    // 1. Drain existing subscriptions gracefully
    if (instance->_sub) {
        instance->_log->info("Draining subscription...");
        natsStatus status = natsSubscription_Drain(instance->_sub);
        if (status == NATS_OK) {
            instance->_log->info("Successfully initiated subscription drain");

            // Wait for drain completion with configurable timeout
            status = natsSubscription_WaitForDrainCompletion(instance->_sub,
                                                             instance->_drainTimeout.count());
            if (status == NATS_OK) {
                instance->_log->info("Subscription drain completed successfully");
            } else {
                instance->_log->warn("Subscription drain timeout or failed: {}",
                                     natsStatus_GetText(status));
            }
        } else {
            instance->_log->warn("Failed to drain subscription: {}", natsStatus_GetText(status));
        }
    }

    // 2. Flush pending messages to current server
    instance->_log->info("Flushing pending messages...");
    natsStatus status = natsConnection_FlushTimeout(
        nc, instance->_lameDuckModeFlushTimeout.count()); // Use configurable timeout
    if (status == NATS_OK) {
        instance->_log->info("Successfully flushed pending messages");
    } else {
        instance->_log->warn("Flush failed: {}", natsStatus_GetText(status));
    }

    // 3. Trigger reconnection to other servers
    instance->_log->info("Initiating reconnection...");
    natsConnection_Reconnect(nc);

    // 4. Reset lame duck mode flag after reconnection completes
    instance->_log->info("=== LAME DUCK MODE HANDLING COMPLETE ===");
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
