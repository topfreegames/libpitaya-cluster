#ifndef PITAYA_GRPC_RPC_SERVER_H
#define PITAYA_GRPC_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/grpc_config.h"
#include "pitaya/protos/pitaya.grpc.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_server.h"
#include "pitaya/utils/sync_vector.h"

#include "spdlog/logger.h"

#include <thread>
#include <atomic>
#include <grpcpp/server.h>

class PitayaGrpcImpl;
class CallData;

namespace pitaya {

class GrpcServer : public RpcServer
{
public:
    GrpcServer(GrpcConfig config, const char* loggerName = nullptr);
    ~GrpcServer();

    void Start(RpcHandlerFunc handler) override;

    void Shutdown() override;

private:
    void ThreadStart();
    void ProcessRpcs(grpc::ServerCompletionQueue* cq, int threadId);
    void ProcessCallData(CallData* callData, grpc::ServerCompletionQueue* cq, int threadId);
    void InvalidateInProcessRpcs();

private:
    std::shared_ptr<spdlog::logger> _log;
    RpcHandlerFunc _handlerFunc;
    std::atomic_bool _shuttingDown;
    GrpcConfig _config;
    std::unique_ptr<grpc::Server> _grpcServer;
    std::unique_ptr<protos::Pitaya::AsyncService> _service;
    std::vector<std::thread> _workerThreads;
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> _completionQueues;

    // Tracks the number of RPCs that are being processed.
    utils::SyncVector<CallData*> _inProcessRpcs;
};

} // namespace pitaya

#endif // PITAYA_GRPC_RPC_SERVER_H
