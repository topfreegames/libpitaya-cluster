#ifndef PITAYA_NATS_CONFIG_H
#define PITAYA_NATS_CONFIG_H

#include <chrono>
#include <string>

namespace pitaya {

struct NatsConfig
{
    // TODO make more nats configs in both client and server
    // like buffer size, connection retries, etc
    std::string natsAddr;
    std::chrono::milliseconds connectionTimeout;
    std::chrono::milliseconds requestTimeout;
    std::chrono::milliseconds serverShutdownDeadline;
    int reconnectWait;
    int serverMaxNumberOfRpcs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;

    NatsConfig(const std::string& addr,
               std::chrono::milliseconds requestTimeout,
               std::chrono::milliseconds connectionTimeout,
               std::chrono::milliseconds serverShutdownDeadline,
               int serverMaxNumberOfRpcs,
               int maxReconnectionAttempts,
               int maxPendingMsgs)
        : natsAddr(addr)
        , connectionTimeout(connectionTimeout)
        , requestTimeout(requestTimeout)
        , serverShutdownDeadline(serverShutdownDeadline)
        , serverMaxNumberOfRpcs(serverMaxNumberOfRpcs)
        , maxReconnectionAttempts(maxReconnectionAttempts)
        , maxPendingMsgs(maxPendingMsgs)
    {}

    NatsConfig()
        : connectionTimeout(0)
        , requestTimeout(0)
        , serverShutdownDeadline(2000)
        , serverMaxNumberOfRpcs(500)
        , maxReconnectionAttempts(30)
        , reconnectWait(2000)
        , maxPendingMsgs(100)
    {}
};

} // namespace pitaya

#endif // PITAYA_NATS_CONFIG_H
