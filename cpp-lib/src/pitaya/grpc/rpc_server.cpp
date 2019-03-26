#include "pitaya/grpc/rpc_server.h"

#include "pitaya/constants.h"
#include "pitaya/protos/pitaya.grpc.pb.h"
#include "pitaya/utils/grpc.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <cpprest/json.h>
#include <functional>
#include <grpcpp/server_builder.h>

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
    explicit PitayaGrpcImpl(pitaya::RpcHandlerFunc handlerFunc, const char* loggerName = nullptr)
        : _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag)
                          : spdlog::stdout_color_mt(kLogTag))
        , _handlerFunc(std::move(handlerFunc))
    {}

    ~PitayaGrpcImpl() { spdlog::drop(kLogTag); }

    grpc::Status Call(grpc::ServerContext* context,
                      const protos::Request* req,
                      protos::Response* res) override
    {
        // TODO: Add support for RPC Sys
        if (req->type() == protos::RPCType::Sys) {
            *res = NewUnsupportedRpcRtype();
            return grpc::Status::OK;
        }

        *res = _handlerFunc(*req);
        return grpc::Status::OK;
    }

private:
    static constexpr const char* kLogTag = "grpc_server_impl";

    std::shared_ptr<spdlog::logger> _log;
    pitaya::RpcHandlerFunc _handlerFunc;
};

namespace pitaya {

static constexpr const char* kLogTag = "grpc_server";

GrpcServer::GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName)
    : RpcServer(handler)
    , _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _config(std::move(config))
    , _service(new PitayaGrpcImpl(_handlerFunc, loggerName))
{
    const auto address = _config.host + ":" + std::to_string(_config.port);

    _log->debug("Creating gRPC server at address {}", address);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(_service.get());

    _grpcServer = std::unique_ptr<grpc::Server>(builder.BuildAndStart());

    if (!_grpcServer) {
        throw PitayaException(fmt::format("Failed to start gRPC server at address {}", address));
    }

    _log->info("gRPC server started at: {}", address);
}

GrpcServer::~GrpcServer()
{
    if (_grpcServer) {
        _log->info("Shutting down gRPC server");
        _grpcServer->Shutdown();
        _grpcServer->Wait();
    }
    spdlog::drop(kLogTag);
}

} // namespace pitaya
