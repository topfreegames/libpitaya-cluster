#include "pitaya/grpc/rpc_server.h"

#include "pitaya/constants.h"
#include "pitaya/protos/pitaya.grpc.pb.h"
#include "pitaya/utils/grpc.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <cpprest/json.h>
#include <functional>
#include <grpcpp/server_builder.h>

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
    , _config(std::move(config))
    , _service(new PitayaGrpcImpl(_handlerFunc, loggerName))
{
    const auto address = _config.host + ":" + std::to_string(_config.port);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());

    unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS,
                                concurentThreadsSupported);
    builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS,
                                concurentThreadsSupported);
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());

    builder.RegisterService(_service.get());

    _grpcServer = std::unique_ptr<grpc::Server>(builder.BuildAndStart());
    if (!_grpcServer) {
        throw PitayaException(fmt::format("Failed to start gRPC server at address {}", address));
    }
    _log = loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag);
    _log->debug("Creating gRPC server at address {} with {} cqs and pollers",
                address,
                concurentThreadsSupported);
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
