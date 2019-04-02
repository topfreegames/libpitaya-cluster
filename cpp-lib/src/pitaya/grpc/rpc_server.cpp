#include "pitaya/grpc/rpc_server.h"

#include "pitaya/constants.h"
#include "pitaya/protos/pitaya.grpc.pb.h"
#include "pitaya/utils/grpc.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <cpprest/json.h>
#include <functional>
#include <grpcpp/server_builder.h>

using namespace grpc;

class CallData : public pitaya::Rpc
{
public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(protos::Pitaya::AsyncService* service,
             grpc::ServerCompletionQueue* cq,
             pitaya::RpcHandlerFunc handler,
             std::shared_ptr<spdlog::logger> log)
        : _log(log)
        , _service(service)
        , _cq(cq)
        , _handlerFunc(std::move(handler))
        , _responder(&_ctx)
        , _status(CREATE)
    {
        // Invoke the serving logic right away.
        assert(service);
        assert(cq);
        Proceed();
    }

    void Proceed()
    {
        if (_status == CREATE) {
            // As part of the initial CREATE state, we *request* that the system
            // start processing SayHello requests. In this request, "this" acts are
            // the tag uniquely identifying the request (so that different CallData
            // instances can serve different requests concurrently), in this case
            // the memory address of this CallData instance.
            _service->RequestCall(&_ctx, &_request, &_responder, _cq, _cq, this);
            // Make this instance progress to the PROCESS state.
            _status = PROCESS;
        } else if (_status == PROCESS) {
            // Spawn a new CallData instance to serve new clients while we process
            // the one for this CallData. The instance will deallocate itself as
            // part of its FINISH state.

            new CallData(_service, _cq, _handlerFunc, _log);

            _handlerFunc(_request, this);
        } else {
            GPR_ASSERT(_status == FINISH);
            // Once in the FINISH state, deallocate ourselves (CallData).
            delete this;
        }
    }

    void Finish(protos::Response res) override
    {
        _status = FINISH;
        _responder.Finish(res, grpc::Status::OK, this);
    }

private:
    std::shared_ptr<spdlog::logger> _log;
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    protos::Pitaya::AsyncService* _service;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* _cq;

    pitaya::RpcHandlerFunc _handlerFunc;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext _ctx;

    // What we get from the client.
    protos::Request _request;
    // What we send back to the client.
    protos::Response _response;

    // The means to get back to the client.
    ServerAsyncResponseWriter<protos::Response> _responder;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus
    {
        CREATE,
        PROCESS,
        FINISH
    };
    CallStatus _status; // The current serving state.
};

namespace pitaya {

static constexpr const char* kLogTag = "grpc_server";

GrpcServer::GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName)
    : RpcServer(handler)
    , _config(std::move(config))
    , _service(new protos::Pitaya::AsyncService())
{
    const auto address = _config.host + ":" + std::to_string(_config.port);

    grpc::ServerBuilder builder;

    auto concurrentThreadsSupported = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < concurrentThreadsSupported; i++) {
        _completionQueues.push_back(builder.AddCompletionQueue());
    }

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(_service.get());
    _grpcServer = std::unique_ptr<grpc::Server>(builder.BuildAndStart());

    if (!_grpcServer) {
        throw PitayaException(fmt::format("Failed to start gRPC server at address {}", address));
    }

    _log = loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag);
    _log->info("gRPC server started at: {}", address);

    for (auto it = _completionQueues.begin(); it != _completionQueues.end(); it++) {
        _workerThreads.push_back(
            new std::thread(std::bind(&GrpcServer::ProcessRpcs, this, (*it).get())));
    }
}

GrpcServer::~GrpcServer()
{
    // NOTE, TODO: The shutdown method cancels all of the current
    // tags immediately, if this is not desidered, a diferent workflow
    // should be thought of (maybe Shutdown with deadline?).
    _grpcServer->Shutdown();
    _grpcServer->Wait();

    for (const auto& queue : _completionQueues) {
        queue->Shutdown();
    }

    for (const auto& thread : _workerThreads) {
        if (thread->joinable()) {
            _log->info("Waiting for worker thread");
            thread->join();
        }
    }

    spdlog::drop(kLogTag);
}

void
GrpcServer::ProcessRpcs(ServerCompletionQueue* cq)
{
    _log->info("Started processing rpcs");
    new CallData(_service.get(), cq, _handlerFunc, _log);
    void* tag; // uniquely identifies a request.
    bool ok;

    for (;;) {
        if (!cq->Next(&tag, &ok)) {
            break;
        }

        if (!ok) {
            cq->Shutdown();
            continue;
        }

        static_cast<CallData*>(tag)->Proceed();
    }
}

} // namespace pitaya
