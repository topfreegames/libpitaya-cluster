#ifndef PITAYA_NATS_CONFIG_H
#define PITAYA_NATS_CONFIG_H

#include <string>

namespace pitaya {
namespace nats {

struct NATSConfig
{
    // TODO make more nats configs in both client and server
    // like buffer size, connection retries, etc
    std::string nats_addr;
    int64_t connection_timeout_ms;
    int request_timeout_ms;
    int max_reconnection_attempts;
    int max_pending_msgs;

    NATSConfig(const std::string& addr,
               const int request_timeout_ms,
               const int64_t connection_timeout_ms,
               const int max_reconnection_attempts,
               const int max_pending_msgs)
        : nats_addr(addr)
        , connection_timeout_ms(connection_timeout_ms)
        , request_timeout_ms(request_timeout_ms)
        , max_reconnection_attempts(max_reconnection_attempts)
        , max_pending_msgs(max_pending_msgs)
    {}
};

}
}

#endif // PITAYA_NATS_CONFIG_H