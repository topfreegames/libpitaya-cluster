#ifndef PITAYA_NATS_RPC_CLIENT_H
#define PITAYA_NATS_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_client.h"
#include "spdlog/spdlog.h"

#include <nats/nats.h>
#include <string>

namespace pitaya {

class NatsRpcClient : public RpcClient
{
public:
    NatsRpcClient(const NatsConfig& config, const char* loggerName = nullptr);
    ~NatsRpcClient();
    protos::Response Call(const pitaya::Server& target, const protos::Request& req) override;
    protos::Response SendPushToUser(const std::string& server_id,
                                    const std::string& server_type,
                                    const protos::Push& push) override;
    protos::KickAnswer SendKickToUser(const std::string& server_id,
                                      const std::string& server_type,
                                      const protos::KickMsg& kick) override;

private:
    std::shared_ptr<spdlog::logger> _log;
    natsOptions* _opts;
    natsConnection* _nc;
    int _timeoutMs;
    static void ClosedCb(natsConnection* nc,
                         void* closure); // called when all reconnection requests failed
    static void DisconnectedCb(natsConnection* nc,
                               void* closure); // called when the connection is lost
    static void ReconnectedCb(natsConnection* nc,
                              void* closure); // called when the connection is repaired
};

} // namespace pitaya

#endif // PITAYA_NATS_RPC_CLIENT_H
