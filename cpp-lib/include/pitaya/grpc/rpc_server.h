#ifndef PITAYA_GRPC_RPC_SERVER_H
#define PITAYA_GRPC_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/grpc/config.h"
#include "protos/pitaya.grpc.pb.h"
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
    GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName = nullptr);
    ~GrpcServer();
    void ThreadStart();

private:

private:
    static constexpr const char* kLogTag = "grpc_server";

    std::shared_ptr<spdlog::logger> _log;
    GrpcConfig _config;

    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    protos::Pitaya::AsyncService service_;
    std::unique_ptr<::grpc::Server> _grpcServer;
  };

} // namespace pitaya

#endif // PITAYA_GRPC_RPC_SERVER_H
