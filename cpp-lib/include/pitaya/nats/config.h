#ifndef PITAYA_NATS_CONFIG_H
#define PITAYA_NATS_CONFIG_H

#include <string>

namespace pitaya {
namespace nats {

struct NATSConfig
{
    // TODO make more nats configs in both client and server
    // like buffer size, connection retries, etc
    std::string natsAddr;
    int64_t connectionTimeoutMs;
    int requestTimeoutMs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;

    NATSConfig(){};

    NATSConfig(const std::string& addr,
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

    NATSConfig()
        : connectionTimeoutMs(-1)
        , requestTimeoutMs(-1)
        , maxReconnectionAttempts(3)
        , maxPendingMsgs(100)
    {}
};

}
}

#endif // PITAYA_NATS_CONFIG_H