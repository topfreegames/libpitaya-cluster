#ifndef PITAYA_NATS_RPC_CLIENT_H
#define PITAYA_NATS_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/nats_config.h"
#include "pitaya/nats_client.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_client.h"
#include "spdlog/spdlog.h"

#include <string>

namespace pitaya {

class NatsRpcClient : public RpcClient
{
public:
    NatsRpcClient(const NatsConfig& config,
                  std::unique_ptr<NatsClient> natsClient,
                  const char* loggerName = nullptr);
    NatsRpcClient(const NatsConfig& config, const char* loggerName = nullptr);
    ~NatsRpcClient();
    protos::Response Call(const pitaya::Server& target, const protos::Request& req) override;
    boost::optional<PitayaError> SendPushToUser(const std::string& serverId,
                                                const std::string& serverType,
                                                const protos::Push& push) override;
    boost::optional<PitayaError> SendKickToUser(const std::string& serverId,
                                                const std::string& serverType,
                                                const protos::KickMsg& kick) override;

private:
    std::shared_ptr<spdlog::logger> _log;
    std::unique_ptr<NatsClient> _natsClient;
    std::chrono::milliseconds _requestTimeout;
};

} // namespace pitaya

#endif // PITAYA_NATS_RPC_CLIENT_H
