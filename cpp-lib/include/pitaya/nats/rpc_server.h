#ifndef PITAYA_NATS_RPC_SERVER_H
#define PITAYA_NATS_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "pitaya/rpc_server.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "spdlog/spdlog.h"

#include <nats/nats.h>
#include <string>

namespace pitaya {
namespace nats {

class NatsRpcServer : public RpcServer
{
public:
    NatsRpcServer(const Server& server,
                  const NatsConfig& config,
                  RpcHandlerFunc handler,
                  const char* loggerName = nullptr);

    ~NatsRpcServer();

private:
    void PrintSubStatus(natsSubscription* sub);
    static void HandleMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure);
    static void ErrHandler(natsConnection* nc,
                           natsSubscription* subscription,
                           natsStatus err,
                           void* closure);
    static void ClosedCb(natsConnection* nc,
                         void* closure); // called when all reconnection requests failed
    static void DisconnectedCb(natsConnection* nc,
                               void* closure); // called when the connection is lost
    static void ReconnectedCb(natsConnection* nc,
                              void* closure); // called when the connection is repaired

private:
    std::shared_ptr<spdlog::logger> _log;
    natsOptions* _opts;
    natsConnection* _nc;
    natsSubscription* _sub;
};

} // namespace nats
} // namespace pitaya

#endif // PITAYA_NATS_RPC_SERVER_H
