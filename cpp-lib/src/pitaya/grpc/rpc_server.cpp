#include "pitaya/grpc/rpc_server.h"

#include "pitaya/constants.h"
#include "pitaya/utils/grpc.h"
#include "protos/pitaya.grpc.pb.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <cpprest/json.h>
#include <functional>
#include <grpcpp/server_builder.h>

using ::grpc::ServerBuilder;

static protos::Response
NewUnsupportedRpcRtype()
{
    auto error = new protos::Error();
    error->set_code(pitaya::constants::kCodeUnprocessableEntity);
    error->set_msg("This server does not support RPC Sys");
    protos::Response res;
    res.set_allocated_error(error);
    return res;
}

class PitayaGrpcImpl final : public protos::Pitaya::Service
{
public:
    PitayaGrpcImpl(pitaya::RpcHandlerFunc handlerFunc, const char* loggerName = nullptr)
        : _log(loggerName ? spdlog::get(loggerName)->clone("pitaya_grpc_rpc_server_impl")
                          : spdlog::stdout_color_mt("pitaya_grpc_rpc_server_impl"))
        , _handlerFunc(std::move(handlerFunc))
    {}

    grpc::Status Call(grpc::ServerContext* context,
                      const protos::Request* req,
                      protos::Response* res) override
    {
        // TODO: Add support for RPC Sys
        if (req->type() == protos::RPCType::Sys) {
            *res = NewUnsupportedRpcRtype();
            return grpc::Status::OK;
        }

        // TODO: do I have to switch on the rpc type? (sys or user)
        *res = _handlerFunc(*req);
        return grpc::Status::OK;
    }

private:
    std::shared_ptr<spdlog::logger> _log;
    pitaya::RpcHandlerFunc _handlerFunc;
};

namespace pitaya {
namespace grpc {

GrpcRpcServer::GrpcRpcServer(const Server& server, RpcHandlerFunc handler, const char* loggerName)
    : RpcServer(handler)
    , _log(loggerName ? spdlog::get(loggerName)->clone("grpc_rpc_server")
                      : spdlog::stdout_color_mt("grpc_rpc_server"))
    , _service(new PitayaGrpcImpl(_handlerFunc, loggerName))
{
    const auto address = utils::GetGrpcAddressFromServer(server);

    _log->debug("Creating gRPC server at address {}", address);

    ServerBuilder builder;
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());
    builder.RegisterService(_service.get());

    _grpcServer = std::unique_ptr<::grpc::Server>(builder.BuildAndStart());
    _log->info("gRPC server started at: {}", address);
}

GrpcRpcServer::~GrpcRpcServer()
{
    _grpcServer->Shutdown();
    _log->info("Shutting down gRPC server");
    _grpcServer->Wait();
}

void
GrpcRpcServer::ThreadStart()
{
    _log->info("Starting grpc rpc server thread");
}

} // namespace grpc
} // namespace pitaya
