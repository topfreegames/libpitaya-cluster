#ifndef PITAYA_GRPC_RPC_SERVER_H
#define PITAYA_GRPC_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/rpc_server.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"

#include "spdlog/logger.h"

#include <grpcpp/server.h>

class PitayaGrpcImpl;

namespace pitaya {

class GrpcServer : public RpcServer
{
public:
    GrpcServer(const Server& server, RpcHandlerFunc handler, const char* loggerName = nullptr);
    ~GrpcServer();

private:
    void ThreadStart();

private:
    std::shared_ptr<spdlog::logger> _log;
    std::unique_ptr<::grpc::Server> _grpcServer;
    std::unique_ptr<PitayaGrpcImpl> _service;
};

} // namespace pitaya

#endif // PITAYA_GRPC_RPC_SERVER_H