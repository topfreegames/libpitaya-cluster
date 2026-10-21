#include "pitaya/nats_client.h"

#include "pitaya/utils.h"

#include <algorithm>
#include <chrono>
#include <mutex>
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
static constexpr std::chrono::milliseconds kSubscriptionPropagationDelay(1000);
static constexpr std::chrono::milliseconds kNewConnectionRetryWindow(3000);
static constexpr std::chrono::milliseconds kNewConnectionRetryDelay(200);

//
// NatsClientImpl
//
NatsClientImpl::Connection::~Connection()
{
    if (sub) {
        natsSubscription_Destroy(sub);
    }
    if (conn) {
        natsConnection_Destroy(conn);
    }
}

NatsClientImpl::NatsClientImpl(NatsApiType apiType,
                               const NatsConfig& config,
                               const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _config(config)
    , _drainTimeout(config.drainTimeout)
    , _lameDuckModeFlushTimeout(config.flushTimeout)
    , _opts(nullptr)
    , _switchOpts(nullptr)
    , _onMessage(nullptr)
    , _shuttingDown(false)
    , _openConnections(0)
    , _switchInProgress(false)
    , _pendingLameDuckConn(nullptr)
{
    if (config.natsAddr.empty()) {
        throw PitayaException("NATS address should not be empty");
    }

    if (config.maxReconnectionAttempts < 0) {
        throw PitayaException("NATS max reconnection attempts should be positive");
    }

    _log->info("NATS Connection Timeout - " + std::to_string(config.connectionTimeout.count()));
    _log->info("NATS Max Reconnect Attempts - " + std::to_string(config.maxReconnectionAttempts));
    _log->info("NATS Reconnect Wait Time - " + std::to_string(config.reconnectWaitInMs.count()));
    _log->info("NATS Reconnect Buffer Size - " + std::to_string(config.reconnectBufSize));
    _log->info("NATS Ping Interval - " + std::to_string(config.pingIntervalInMs.count()));
    _log->info("NATS Max Pings Out - " + std::to_string(config.maxPingsOut));
    _log->info("NATS Reconnect Jitter - " + std::to_string(config.reconnectJitterInMs.count()));

    // Only enable RetryOnFailedConnect if maxReconnectionAttempts > 0
    // Otherwise the initial connection will hang indefinitely
    _opts = CreateOptions(apiType, config.maxReconnectionAttempts > 0);
    _switchOpts = CreateOptions(apiType, false);
    if (!_opts || !_switchOpts) {
        natsOptions_Destroy(_opts);
        natsOptions_Destroy(_switchOpts);
        throw PitayaException("error configuring nats client");
    }

    // TODO, FIXME: Change so that connection happens NOT in the constructor.
    natsConnection* conn = nullptr;
    natsStatus status = natsConnection_Connect(&conn, _opts);
    if (status != NATS_OK) {
        natsOptions_Destroy(_opts);
        natsOptions_Destroy(_switchOpts);
        std::string err_str("unable to initialize nats server - ");
        err_str.append(natsStatus_GetText(status));
        throw PitayaException(err_str);
    }
    ++_openConnections;
    auto active = std::make_shared<Connection>();
    active->conn = conn;
    std::lock_guard<std::mutex> lock(_activeMutex);
    _active = std::move(active);
}

natsOptions*
NatsClientImpl::CreateOptions(NatsApiType apiType, bool retryOnFailedConnect)
{
    natsOptions* opts = nullptr;
    natsStatus status = natsOptions_Create(&opts);
    if (status != NATS_OK) {
        _log->error("error configuring nats client - {}", natsStatus_GetText(status));
        return nullptr;
    }

    natsOptions_SetTimeout(opts, _config.connectionTimeout.count());
    natsOptions_SetMaxReconnect(opts, _config.maxReconnectionAttempts);

    // Reconnect parameters
    natsOptions_SetReconnectBufSize(opts, _config.reconnectBufSize);
    natsOptions_SetReconnectWait(opts, _config.reconnectWaitInMs.count());
    natsOptions_SetRetryOnFailedConnect(opts, retryOnFailedConnect, NULL, this);
    natsOptions_SetPingInterval(opts, _config.pingIntervalInMs.count());
    natsOptions_SetMaxPingsOut(opts, _config.maxPingsOut);
    natsOptions_SetReconnectJitter(
        opts, _config.reconnectJitterInMs.count(), _config.reconnectJitterInMs.count());

    // Callbacks
    natsOptions_SetClosedCB(opts, ClosedCb, this);
    natsOptions_SetDisconnectedCB(opts, DisconnectedCb, this);
    natsOptions_SetReconnectedCB(opts, ReconnectedCb, this);
    natsOptions_SetLameDuckModeCB(opts, LameDuckModeCb, this);

    if (apiType == NatsApiType::Asynchronous) {
        natsOptions_SetMaxPendingMsgs(opts, _config.maxPendingMsgs);
        natsOptions_SetErrorHandler(opts, ErrHandler, this);
    }
    natsOptions_SetURL(opts, _config.natsAddr.c_str());
    return opts;
}

NatsClientImpl::~NatsClientImpl()
{
    _shuttingDown = true;

    std::thread switchThread;
    {
        std::lock_guard<std::mutex> lock(_switchMutex);
        switchThread = std::move(_switchThread);
    }
    if (switchThread.joinable()) {
        switchThread.join();
    }

    // Only the switch thread replaces the active connection, so it does not change anymore.
    std::shared_ptr<Connection> active;
    {
        std::lock_guard<std::mutex> lock(_activeMutex);
        active = _active;
    }
    // A completed drain also means that no message callback is still running.
    DrainSubscription(*active);
    natsConnection_Close(active->conn);
    active.reset();
    {
        std::lock_guard<std::mutex> lock(_activeMutex);
        _active.reset();
    }

    // The callbacks of every connection use this instance, so wait until the last one ran.
    while (_openConnections > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    natsOptions_Destroy(_opts);
    natsOptions_Destroy(_switchOpts);
}

std::shared_ptr<NatsClientImpl::Connection>
NatsClientImpl::AcquireActiveConnection()
{
    // The count is taken under the lock, so that the switch thread sees every request that
    // can still use a connection after it replaced it.
    std::lock_guard<std::mutex> lock(_activeMutex);
    ++_active->inFlight;
    return _active;
}

natsStatus
NatsClientImpl::Request(std::shared_ptr<NatsMsg>* msg,
                        const std::string& topic,
                        const std::vector<uint8_t>& data,
                        std::chrono::milliseconds timeout)
{
    auto connection = AcquireActiveConnection();
    natsMsg* reply = nullptr;
    natsStatus status = natsConnection_Request(
        &reply, connection->conn, topic.c_str(), data.data(), data.size(), timeout.count());
    --connection->inFlight;

    if (status == NATS_OK) {
        *msg = std::shared_ptr<NatsMsg>(new NatsMsgImpl(reply));
    }
    return status;
}

natsStatus
NatsClientImpl::Subscribe(const std::string& topic,
                          std::function<void(std::shared_ptr<NatsMsg>)> onMessage)
{
    // Holding the switch lock keeps a lame duck switch from starting meanwhile.
    std::lock_guard<std::mutex> switchLock(_switchMutex);
    if (_switchInProgress) {
        _log->warn("Attempting to subscribe during lame duck mode - operation skipped");
        return NATS_ILLEGAL_STATE;
    }

    _log->info("Subscribing to topic {}", topic);
    _onMessage = std::move(onMessage);

    // The topic is also the queue name, so that the subscriptions of the old and the new
    // connection can coexist during a lame duck switch without delivering a message twice.
    std::lock_guard<std::mutex> lock(_activeMutex);
    natsStatus status = natsConnection_QueueSubscribe(
        &_active->sub, _active->conn, topic.c_str(), topic.c_str(), HandleMsg, this);
    if (status != NATS_OK) {
        _active->sub = nullptr;
        _log->error("Failed to subscribe");
        return status;
    }
    _topic = topic;
    return status;
}

bool
NatsClientImpl::IsInLameDuckMode() const
{
    return _switchInProgress;
}

natsStatus
NatsClientImpl::Publish(const char* reply, const std::vector<uint8_t>& buf)
{
    auto connection = AcquireActiveConnection();
    natsStatus status = natsConnection_Publish(connection->conn, reply, buf.data(), buf.size());
    --connection->inFlight;

    if (status != NATS_OK) {
        switch (status) {
            case NATS_TIMEOUT:
                _log->warn("NATS publish timeout");
                return status;
            case NATS_INSUFFICIENT_BUFFER:
                _log->warn("NATS reconnection buffer full - message dropped");
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
    bool isActiveConnection;
    {
        std::lock_guard<std::mutex> lock(instance->_activeMutex);
        isActiveConnection = instance->_active && instance->_active->conn == nc;
    }
    // The destructor clears the active connection right after it closes it.
    if (isActiveConnection || instance->_shuttingDown) {
        instance->_log->warn("nats disconnected");
    } else {
        instance->_log->info("Replaced nats connection disconnected");
    }
}

void
NatsClientImpl::ReconnectedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    // NATS.c sends the subscriptions of the connection again after a reconnection.
    instance->_log->warn("nats reconnected!");
}

void
NatsClientImpl::ClosedCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);
    bool isActiveConnection;
    {
        std::lock_guard<std::mutex> lock(instance->_activeMutex);
        isActiveConnection = instance->_active && instance->_active->conn == nc;
    }
    bool shuttingDown = instance->_shuttingDown;
    if (isActiveConnection || shuttingDown) {
        instance->_log->warn("nats connection closed!");
    } else {
        instance->_log->warn("Replaced nats connection closed");
    }

    // The destructor can free the instance as soon as this count reaches zero, so this must
    // be the last access to it.
    --instance->_openConnections;

    // A connection that was replaced in a lame duck switch is closed on purpose.
    if (isActiveConnection && !shuttingDown) {
        // This was not initiated by a shutdown request.
        // Send SIGTERM as this was caused by all reconnection attempts failing
        utils::RequestProcessTermination();
    }
}

void
NatsClientImpl::LameDuckModeCb(natsConnection* nc, void* user)
{
    auto instance = reinterpret_cast<NatsClientImpl*>(user);

    instance->_log->warn("=== LAME DUCK MODE DETECTED ===");
    char serverUrl[256];
    natsConnection_GetConnectedUrl(nc, serverUrl, sizeof(serverUrl));
    instance->_log->debug("Server: {}", serverUrl);

    // The switch runs on its own thread, because this callback runs on the NATS thread that
    // serves the callbacks of all connections.
    std::lock_guard<std::mutex> switchLock(instance->_switchMutex);
    if (instance->_shuttingDown) {
        return;
    }
    if (instance->_switchInProgress) {
        // The connection can be the new one of the switch in progress. The switch thread
        // checks it when it finishes.
        instance->_log->warn("Lame duck switch in progress - checking this connection after it");
        instance->_pendingLameDuckConn = nc;
        return;
    }
    {
        // _active is null only while the constructor has not finished.
        std::lock_guard<std::mutex> lock(instance->_activeMutex);
        if (!instance->_active || instance->_active->conn != nc) {
            instance->_log->warn("Connection in lame duck mode is not the active one - ignoring");
            return;
        }
    }
    instance->_switchInProgress = true;
    std::thread previousSwitchThread = std::move(instance->_switchThread);
    instance->_switchThread = std::thread(
        &NatsClientImpl::SwitchConnection, instance, std::move(previousSwitchThread));
}

void
NatsClientImpl::SwitchConnection(std::thread previousSwitchThread)
{
    // The previous switch has already finished, except for its last instructions.
    if (previousSwitchThread.joinable()) {
        previousSwitchThread.join();
    }

    for (;;) {
        bool switched = SwitchOnce();

        // If the new connection entered lame duck mode during the switch, switch again.
        std::lock_guard<std::mutex> switchLock(_switchMutex);
        natsConnection* pending = _pendingLameDuckConn;
        _pendingLameDuckConn = nullptr;
        bool switchAgain = false;
        if (switched && pending && !_shuttingDown) {
            std::lock_guard<std::mutex> lock(_activeMutex);
            switchAgain = (_active->conn == pending);
        }
        if (!switchAgain) {
            _switchInProgress = false;
            return;
        }
        _log->warn("New connection entered lame duck mode during the switch - switching again");
    }
}

bool
NatsClientImpl::SwitchOnce()
{
    std::shared_ptr<Connection> old;
    std::string topic;
    {
        std::lock_guard<std::mutex> lock(_activeMutex);
        old = _active;
        topic = _topic;
    }

    // 1. Open a new connection. A server in lame duck mode does not accept new connections,
    // so it connects to another server. The NATS address can still send a connection to the
    // server in lame duck mode, so retry for a short time, well inside the grace period.
    auto fresh = std::make_shared<Connection>();
    auto connectUntil = std::chrono::steady_clock::now() + kNewConnectionRetryWindow;
    natsStatus status;
    for (;;) {
        status = natsConnection_Connect(&fresh->conn, _switchOpts);
        if (status == NATS_OK) {
            ++_openConnections;
            break;
        }
        fresh->conn = nullptr;
        if (_shuttingDown || std::chrono::steady_clock::now() >= connectUntil) {
            break;
        }
        _log->warn("Failed to open a new NATS connection ({}) - retrying",
                   natsStatus_GetText(status));
        std::this_thread::sleep_for(kNewConnectionRetryDelay);
    }

    // 2. Subscribe the new connection, and flush so that the server confirms the
    // subscription before the old one is drained.
    if (status == NATS_OK && !topic.empty()) {
        status = natsConnection_QueueSubscribe(
            &fresh->sub, fresh->conn, topic.c_str(), topic.c_str(), HandleMsg, this);
        if (status != NATS_OK) {
            fresh->sub = nullptr;
        } else {
            status = natsConnection_FlushTimeout(fresh->conn, FlushTimeoutMs());
        }
    }

    if (status != NATS_OK || _shuttingDown) {
        // Keep the old connection. When the server closes it, NATS.c reconnects it to
        // another server and sends its subscription again. The new subscription can
        // already have received messages, so drain it.
        _log->error("Lame duck switch failed ({}) - keeping the current connection",
                    natsStatus_GetText(status));
        DrainSubscription(*fresh);
        fresh.reset();
        return false;
    }

    // 3. Make the new connection the active one. New requests and publishes use it.
    {
        std::lock_guard<std::mutex> lock(_activeMutex);
        _active = fresh;
    }
    _log->warn("Switched to a new NATS connection");

    // If the new connection closed before it became the active one, its ClosedCb did not
    // stop the process. Nothing would receive the messages, so stop it here.
    if (natsConnection_IsClosed(fresh->conn) && !_shuttingDown) {
        _log->critical("New NATS connection closed during the lame duck switch");
        utils::RequestProcessTermination();
    }

    // 4. Let the new subscription reach the whole cluster, then drain the old one. Both
    // subscriptions are in the same queue group, so each message is delivered once.
    if (old->sub) {
        auto drainAt = std::chrono::steady_clock::now() + kSubscriptionPropagationDelay;
        while (!_shuttingDown && std::chrono::steady_clock::now() < drainAt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    auto deadline = std::chrono::steady_clock::now() +
                    std::max(std::chrono::milliseconds(DrainTimeoutMs()), _config.requestTimeout);
    DrainSubscription(*old);

    // 5. Wait for the requests and publishes that still use the old connection, so that their
    // replies are not lost.
    while (old->inFlight > 0 && !_shuttingDown && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (old->inFlight > 0) {
        _log->warn("Closing the old connection with {} requests still in flight",
                   old->inFlight.load());
    }

    // 6. Flush and close the old connection. If the old connection is reconnecting, the
    // flush waits for the reconnection, because Close only flushes a connected connection.
    // The connection is destroyed when the last request that uses it releases it.
    status = natsConnection_FlushTimeout(old->conn, FlushTimeoutMs());
    if (status != NATS_OK) {
        _log->warn("Failed to flush the old connection: {}", natsStatus_GetText(status));
    }
    natsConnection_Close(old->conn);
    old.reset();

    _log->warn("=== LAME DUCK MODE HANDLING COMPLETE ===");
    return true;
}

void
NatsClientImpl::DrainSubscription(Connection& connection)
{
    natsSubscription* sub = connection.sub;
    connection.sub = nullptr;
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
