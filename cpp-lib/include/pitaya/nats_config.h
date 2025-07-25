#ifndef PITAYA_NATS_CONFIG_H
#define PITAYA_NATS_CONFIG_H

#include <chrono>
#include <string>

// Pitaya default values overriding nats defaults
#define PITAYA_NATS_DEFAULT_CONNECTION_TIMEOUT_IN_MS 2000
#define PITAYA_NATS_DEFAULT_REQUEST_TIMEOUT_IN_MS 2000
#define PITAYA_NATS_DEFAULT_SERVER_SHUTDOWN_DEADLINE_IN_MS 2000
#define PITAYA_NATS_DEFAULT_SERVER_MAX_NUMBER_OF_RPCS 500
#define PITAYA_NATS_DEFAULT_MAX_RECONNECTION_ATTEMPTS 30
#define PITAYA_NATS_DEFAULT_MAX_PENDING_MSGS 100

// Reconnection parameters
#define PITAYA_NATS_DEFAULT_RECONNECT_WAIT_IN_MS 100
#define PITAYA_NATS_DEFAULT_RECONNECT_BUF_SIZE 8 * 1024 * 1024
#define PITAYA_NATS_DEFAULT_RECONNECT_JITTER_IN_MS 50
#define PITAYA_NATS_DEFAULT_PING_INTERVAL_IN_MS 1000
#define PITAYA_NATS_DEFAULT_MAX_PINGS_OUT 2

namespace pitaya {

struct NatsConfig
{
    std::string natsAddr;
    std::chrono::milliseconds connectionTimeout;
    std::chrono::milliseconds requestTimeout;
    std::chrono::milliseconds serverShutdownDeadline;
    int serverMaxNumberOfRpcs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;
    // Reconnect parameters
    int reconnectBufSize;
    std::chrono::milliseconds reconnectWaitInMs;
    std::chrono::milliseconds reconnectJitterInMs;
    std::chrono::milliseconds pingIntervalInMs;
    int maxPingsOut;

    NatsConfig(const std::string& addr,
               std::chrono::milliseconds requestTimeout,
               std::chrono::milliseconds connectionTimeout,
               std::chrono::milliseconds serverShutdownDeadline,
               int serverMaxNumberOfRpcs,
               int maxReconnectionAttempts,
               int maxPendingMsgs,
               std::chrono::milliseconds reconnectWaitInMs,
               int reconnectBufSize,
               std::chrono::milliseconds reconnectJitterInMs,
               std::chrono::milliseconds pingIntervalInMs,
               int maxPingsOut)
        : natsAddr(addr)
        , connectionTimeout(connectionTimeout)
        , requestTimeout(requestTimeout)
        , serverShutdownDeadline(serverShutdownDeadline)
        , serverMaxNumberOfRpcs(serverMaxNumberOfRpcs)
        , maxReconnectionAttempts(maxReconnectionAttempts)
        , maxPendingMsgs(maxPendingMsgs)
        , reconnectWaitInMs(reconnectWaitInMs)
        , reconnectBufSize(reconnectBufSize)
        , reconnectJitterInMs(reconnectJitterInMs)
        , pingIntervalInMs(pingIntervalInMs)
        , maxPingsOut(maxPingsOut)
    {
    }

    NatsConfig()
        : connectionTimeout(std::chrono::milliseconds(PITAYA_NATS_DEFAULT_CONNECTION_TIMEOUT_IN_MS))
        , requestTimeout(std::chrono::milliseconds(PITAYA_NATS_DEFAULT_REQUEST_TIMEOUT_IN_MS))
        , serverShutdownDeadline(
              std::chrono::milliseconds(PITAYA_NATS_DEFAULT_SERVER_SHUTDOWN_DEADLINE_IN_MS))
        , serverMaxNumberOfRpcs(PITAYA_NATS_DEFAULT_SERVER_MAX_NUMBER_OF_RPCS)
        , maxReconnectionAttempts(PITAYA_NATS_DEFAULT_MAX_RECONNECTION_ATTEMPTS)
        , maxPendingMsgs(PITAYA_NATS_DEFAULT_MAX_PENDING_MSGS)
        , reconnectWaitInMs(std::chrono::milliseconds(PITAYA_NATS_DEFAULT_RECONNECT_WAIT_IN_MS))
        , reconnectBufSize(PITAYA_NATS_DEFAULT_RECONNECT_BUF_SIZE)
        , reconnectJitterInMs(std::chrono::milliseconds(PITAYA_NATS_DEFAULT_RECONNECT_JITTER_IN_MS))
        , pingIntervalInMs(std::chrono::milliseconds(PITAYA_NATS_DEFAULT_PING_INTERVAL_IN_MS))
        , maxPingsOut(PITAYA_NATS_DEFAULT_MAX_PINGS_OUT)
    {
    }
};

} // namespace pitaya

#endif // PITAYA_NATS_CONFIG_H
