#ifndef PITAYA_GRPC_RPC_SERVER_H
#define PITAYA_GRPC_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/grpc/config.h"
#include "pitaya/protos/pitaya.grpc.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/rpc_server.h"

#include "spdlog/logger.h"

#include <grpcpp/server.h>

class PitayaGrpcImpl;

namespace pitaya {

class GrpcServer : public RpcServer
{
public:
    GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName = nullptr);
    ~GrpcServer();
    void ThreadStart();

private:
    void ProcessRpcs(grpc::ServerCompletionQueue* cq);

private:
    std::shared_ptr<spdlog::logger> _log;
    GrpcConfig _config;
    std::unique_ptr<grpc::Server> _grpcServer;
    std::unique_ptr<protos::Pitaya::AsyncService> _service;
    std::vector<std::thread*> _workerThreads;
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> _completionQueues;
};

} // namespace pitaya

#endif // PITAYA_GRPC_RPC_SERVER_H
