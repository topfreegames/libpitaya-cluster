#ifndef PITAYA_NATS_RPC_CLIENT_H
#define PITAYA_NATS_RPC_CLIENT_H

#include "nats/nats.h"
#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/spdlog.h"
#include <string>

namespace pitaya {
namespace nats {

class NATSRPCClient
{
public:
    NATSRPCClient(const pitaya::Server& server,
                  const NATSConfig& config,
                  const char* loggerName = nullptr);
    std::shared_ptr<protos::Response> Call(const pitaya::Server& target,
                                           std::unique_ptr<protos::Request> req);

private:
    std::shared_ptr<spdlog::logger> _log;
    natsConnection* nc;
//    natsSubscription* sub;
    int timeout_ms;
    static void closed_cb(natsConnection* nc,
                          void* closure); // called when all reconnection requests failed
    static void disconnected_cb(natsConnection* nc,
                                void* closure); // called when the connection is lost
    static void reconnected_cb(natsConnection* nc,
                               void* closure); // called when the connection is repaired
};

} // namespace nats
} // namespace pitaya

#endif // PITAYA_NATS_RPC_CLIENT_H
