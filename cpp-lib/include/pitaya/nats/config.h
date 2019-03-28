#ifndef PITAYA_NATS_CONFIG_H
#define PITAYA_NATS_CONFIG_H

#include <string>

namespace pitaya {

struct NatsConfig
{
    // TODO make more nats configs in both client and server
    // like buffer size, connection retries, etc
    std::string natsAddr;
    int64_t connectionTimeoutMs;
    int requestTimeoutMs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;

    NatsConfig(const std::string& addr,
               const int requestTimeoutMs,
               const int64_t connectionTimeoutMs,
               const int maxReconnectionAttempts,
               const int maxPendingMsgs)
        : natsAddr(addr)
        , connectionTimeoutMs(connectionTimeoutMs)
        , requestTimeoutMs(requestTimeoutMs)
        , maxReconnectionAttempts(maxReconnectionAttempts)
        , maxPendingMsgs(maxPendingMsgs)
    {}

    NatsConfig()
        : connectionTimeoutMs(-1)
        , requestTimeoutMs(-1)
        , maxReconnectionAttempts(3)
        , maxPendingMsgs(100)
    {}
};

} // namespace pitaya

#endif // PITAYA_NATS_CONFIG_H
