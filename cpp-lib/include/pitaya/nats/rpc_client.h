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

class RPCClient
{
public:
    RPCClient(const pitaya::Server& server,
              const NATSConfig& config,
              const char* loggerName = nullptr);
    protos::Response Call(const pitaya::Server& target, const protos::Request& req);

private:
    std::shared_ptr<spdlog::logger> _log;
    natsConnection* _nc;
    int _timeoutMs;
    static void ClosedCb(natsConnection* nc,
                         void* closure); // called when all reconnection requests failed
    static void DisconnectedCb(natsConnection* nc,
                               void* closure); // called when the connection is lost
    static void ReconnectedCb(natsConnection* nc,
                              void* closure); // called when the connection is repaired
};

} // namespace nats
} // namespace pitaya

#endif // PITAYA_NATS_RPC_CLIENT_H
