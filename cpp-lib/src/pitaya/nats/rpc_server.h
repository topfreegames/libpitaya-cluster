#ifndef PITAYA_NATS_RPC_SERVER_H
#define PITAYA_NATS_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/nats_client.h"
#include "pitaya/nats_config.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_server.h"
#include "pitaya/utils/sync_vector.h"
#include "spdlog/spdlog.h"

#include <nats/nats.h>
#include <string>

namespace pitaya {

class NatsRpcServer : public RpcServer
{
public:
    NatsRpcServer(const Server& server, const NatsConfig& config, const char* loggerName = nullptr);
    NatsRpcServer(const Server& server,
                  const NatsConfig& config,
                  std::unique_ptr<NatsClient> natsClient,
                  const char* loggerName = nullptr);

    ~NatsRpcServer();

    void Start(RpcHandlerFunc handler) override;

    void Shutdown() override;

private:
    struct CallData;

    void PrintSubStatus(natsSubscription* sub);
    void OnNewMessage(std::shared_ptr<NatsMsg> msg);
    void OnRpcFinished(const char* reply, std::vector<uint8_t> responseBuf, CallData* callData);

private:
    std::shared_ptr<spdlog::logger> _log;
    NatsConfig _config;
    std::unique_ptr<NatsClient> _natsClient;
    RpcHandlerFunc _handlerFunc;
    Server _server;
    static std::atomic_int _cnt;

    // Tracks the number of RPCs that are being processed.
    utils::SyncVector<CallData*> _inProcessRpcs;
};

} // namespace pitaya

#endif // PITAYA_NATS_RPC_SERVER_H
