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
    std::atomic_bool isValid;

    explicit CallData()
        : status(Status::Create)
        , responder(&ctx)
        , isValid(true)
    {}

    void Finish(protos::Response res) override
    {
        // TODO: use the right memory order.
        if (isValid) {
            status = Status::Finish;
            responder.Finish(res, grpc::Status::OK, this);
        } else {
            // NOTE(leo): The case where a CallData is not valid is whenever the
            // server is Shutdown and Finish is called after that. In such cases,
            // we cannot call `responder.Finish` anymore, since the server was destroyed.
            // We then only delete the memory to avoid a leak.
            delete this;
        }
    }
};

namespace pitaya {

static constexpr const char* kLogTag = "grpc_server";

GrpcServer::GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName)
    : RpcServer(handler)
    , _shuttingDown(false)
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
    _log->info(
        "gRPC server started at {} with {} grpc threads", address, concurrentThreadsSupported);

    for (size_t i = 0; i < _completionQueues.size(); ++i) {
        _workerThreads.emplace_back(
            std::bind(&GrpcServer::ProcessRpcs, this, _completionQueues[i].get(), i + 1));
    }
}

GrpcServer::~GrpcServer()
{
    using namespace std::chrono;

    // Signal other threads that the server is shutting down.
    // Now, we wait a little bit in order for the exiting rpcs to
    // finish. If the deadline passes, the current rpcs are removed and destroyed.
    // TODO: use the right memory order here.
    _shuttingDown.store(true);

    // The shutdown method cancels all of the current
    // tags immediately.
    _grpcServer->Shutdown(system_clock::now() + _config.serverShutdownDeadline);
    _grpcServer->Wait();

    // Shutdown every completion queue and wait for all of the completion queue
    // threads to join.
    for (const auto& queue : _completionQueues) {
        queue->Shutdown();
    }

    for (auto& thread : _workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    {
        std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
        if (_inProcessRpcs.Size() > 0) {
            _log->warn("Server is shutting down with {} rpcs being processed",
                       _inProcessRpcs.Size());
        }
    }

    _log->info("Shutdown complete");
    spdlog::drop(kLogTag);
}

void
GrpcServer::ProcessRpcs(ServerCompletionQueue* cq, int threadId)
{
    // _log->info("Started processing rpcs on thread {}", threadId);

    // Request the first rpc so that the first tag can be
    // received from Next.
    auto callData = new CallData();
    ProcessCallData(callData, cq, threadId);

    for (;;) {
        void* tag; // uniquely identifies a request.
        bool ok;

        // _log->debug("[thread {}] Waiting for completion queue", threadId);
        if (!cq->Next(&tag, &ok)) {
            // _log->debug("[thread {}] Completion queue was shut down for thread", threadId);
            InvalidateInProcessRpcs();
            break;
        }

        auto callData = static_cast<CallData*>(tag);

        if (!ok) {
            assert(_shuttingDown);
            if (callData->status == CallData::Status::Process) {
                _log->debug("[thread {}] RPC could not be started, server is shutting down",
                            threadId);
            } else if (callData->status == CallData::Status::Finish) {
                _log->warn("[thread {}] RPC could not be finished, server is shutting down",
                           threadId);
            } else {
                assert(false);
            }
            delete callData;
            continue;
        }

        // _log->debug("[thread {}] Got a new tag", threadId);

        ProcessCallData(callData, cq, threadId);
    }
}

void
GrpcServer::ProcessCallData(CallData* callData, ServerCompletionQueue* cq, int threadId)
{
    assert(callData);
    assert(cq);

    switch (callData->status) {
        case CallData::Status::Create: {
            _log->debug("[thread {}] CREATE", threadId);
            // Request for a new Call RPC from the pitaya async service.
            _service->RequestCall(
                &callData->ctx, &callData->request, &callData->responder, cq, cq, callData);
            callData->status = CallData::Status::Process;
            break;
        }
        case CallData::Status::Process: {
            // While we process the current call data, we start requesting another
            // one from the service if the server is not shutting down.
            // TODO: use the correct memory order for _shuttingDown.
            if (!_shuttingDown.load()) {
                // _log->debug("[thread {}] Adding new call data at thread", threadId);
                auto nextCallData = new CallData();
                ProcessCallData(nextCallData, cq, threadId);
            }

            // We lock the current in process RPCs vector and check wether there is enough space
            // to process another RPC.
            std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);

            const bool slotsAvailable = _inProcessRpcs.Size() < _config.serverMaxNumberOfRpcs;
            const bool infiniteSlots = _config.serverMaxNumberOfRpcs == -1;

            if (infiniteSlots || slotsAvailable) {
                _inProcessRpcs.PushBack(callData);
                _log->debug("[thread {}] Will start processing the next RPC ({}/{} rpcs)",
                            threadId,
                            _inProcessRpcs.Size(),
                            _config.serverMaxNumberOfRpcs);
                _handlerFunc(callData->request, callData);
            } else {
                _log->warn("The server is under maximum load, cannot process RPC");
                // There are no space for processing RPCs anymore. We then just return an error
                // to the client.
                auto err = new protos::Error();
                err->set_code(constants::kCodeServiceUnavailable);
                err->set_msg("The server is under maximum load, cannot process RPC");

                protos::Response errorRes;
                errorRes.set_allocated_error(err);

                callData->Finish(errorRes);
            }

            break;
        }
        case CallData::Status::Finish: {
            // _log->debug("[thread {}] FINISH", threadId);
            // The RPC was finished. Therefore we remove from the _inProcessRpcs vector.
            std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
            auto it = std::find(_inProcessRpcs.begin(), _inProcessRpcs.end(), callData);
            if (it != _inProcessRpcs.end()) {
                _inProcessRpcs.Erase(it);
                // _log->debug("[thread {}] Decreasing number of in process rpcs: {}",
                //             threadId,
                //             _inProcessRpcs.Size());
            }

            // The RPC was finished. Therefore we delete the CallData instance.
            delete callData;
            break;
        }
    }
}

void
GrpcServer::InvalidateInProcessRpcs()
{
    std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
    if (_inProcessRpcs.Size() == 0) {
        return;
    }

    for (auto rpc : _inProcessRpcs) {
        // TODO: use the right memory order here instead of the
        // strongest one.
        rpc->isValid.store(false);
    }

    _inProcessRpcs.Clear();
}

} // namespace pitaya
