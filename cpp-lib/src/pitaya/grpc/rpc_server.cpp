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
    enum class Status
    {
        Create,
        Process,
        Finish,
    };

    Status status;
    ServerContext ctx;
    protos::Request request;
    protos::Response response;
    ServerAsyncResponseWriter<protos::Response> responder;

    explicit CallData()
        : responder(&ctx)
        , status(Status::Create)
    {}

    void Finish(protos::Response res) override
    {
        // TODO, FIXME: consider when the instance was already
        // deleted and this function is called. This will probably
        // crash the program. A possible solution is using a shared pointer
        // for CallData.
        status = Status::Finish;
        responder.Finish(res, grpc::Status::OK, this);
    }
};

namespace pitaya {

static constexpr const char* kLogTag = "grpc_server";

GrpcServer::GrpcServer(GrpcConfig config,
                       RpcHandlerFunc handler,
                       const char* loggerName)
    : RpcServer(handler)
    , _shuttingDown(false)
    , _config(std::move(config))
    , _service(new protos::Pitaya::AsyncService())
    , _numInProcessRpcs(0)
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
    using namespace std::chrono;

    // Signal other threads that the server is shutting down.
    // Now, we wait a little bit in order for the exiting rpcs to
    // finish. If the deadline passes, the current rpcs are removed and destroyed.
    _shuttingDown = true;

    // The shutdown method cancels all of the current
    // tags immediately.
    _grpcServer->Shutdown(system_clock::now() + _config.serverShutdownDeadline);

    // Shutdown every completion queue and wait for all of the completion queue
    // threads to join.
    for (const auto& queue : _completionQueues) {
        queue->Shutdown();
    }

    for (const auto& thread : _workerThreads) {
        if (thread->joinable()) {
            thread->join();
        }
    }

    if (_numInProcessRpcs > 0) {
        _log->warn("Server is shutting down with {} rpcs being processed", _numInProcessRpcs);
    }

    _log->info("Shutdown complete");
    spdlog::drop(kLogTag);
}

void
GrpcServer::ProcessRpcs(ServerCompletionQueue* cq)
{
    _log->info("Started processing rpcs");

    // Request the first rpc so that the first tag can be
    // received from Next.
    auto callData = new CallData();
    ProcessCallData(callData, cq);

    for (;;) {
        void* tag; // uniquely identifies a request.
        bool ok;

        if (!cq->Next(&tag, &ok)) {
            _log->debug("Completion queue was shut down for thread");
            break;
        }

        auto callData = static_cast<CallData*>(tag);

        if (!ok) {
            assert(_shuttingDown);
            if (callData->status == CallData::Status::Process) {
                _log->debug("RPC could not be started, server is shutting down");
            } else if (callData->status == CallData::Status::Finish) {
                _log->warn("RPC could not be finished, server is shutting down");
            } else {
                assert(false);
            }
            delete callData;
            continue;
        }

        ProcessCallData(callData, cq);
    }
}

void
GrpcServer::ProcessCallData(CallData* callData, ServerCompletionQueue* cq)
{
    assert(callData);
    assert(cq);

    switch (callData->status) {
        case CallData::Status::Create: {
            // Request for a new Call RPC from the pitaya async service.
            _service->RequestCall(
                &callData->ctx, &callData->request, &callData->responder, cq, cq, callData);
            callData->status = CallData::Status::Process;
            break;
        }
        case CallData::Status::Process: {
            // While we process the current call data, we start requesting another
            // one from the service if the server is not shutting down.
            if (!_shuttingDown) {
                auto nextCallData = new CallData();
                ProcessCallData(nextCallData, cq);
            }
            // Another RPC will start to be processed
            ++_numInProcessRpcs;
            _log->debug("Increasing number of in process rpcs: {}", _numInProcessRpcs);

            // TODO: have some way of specifying a deadline for the client to complete
            // the request.
            _handlerFunc(callData->request, callData);
            break;
        }
        case CallData::Status::Finish: {
            // The RPC was finished. Therefore we decrement
            // the count and delete the CallData instance.
            delete callData;
            --_numInProcessRpcs;
            _log->debug("Decreasing number of in process rpcs: {}", _numInProcessRpcs);
            break;
        }
    }
}

} // namespace pitaya
