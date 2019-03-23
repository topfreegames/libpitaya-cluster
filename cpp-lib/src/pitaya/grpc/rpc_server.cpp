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
using namespace grpc;

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

class PitayaGrpcCall final{
public:
  PitayaGrpcCall(pitaya::RpcHandlerFunc handlerFunc, protos::Pitaya::AsyncService* service, ServerCompletionQueue* cq)
    : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE), _handlerFunc(std::move(handlerFunc)){
      Proceed();
    }

  void Proceed() {
    if (status_ == CREATE) {
      // As part of the initial CREATE state, we *request* that the system
      // start processing SayHello requests. In this request, "this" acts are
      // the tag uniquely identifying the request (so that different CallData
      // instances can serve different requests concurrently), in this case
      // the memory address of this CallData instance.
      service_->RequestCall(&ctx_, &request_, &responder_, cq_, cq_,
          this);
      // Make this instance progress to the PROCESS state.
      status_ = PROCESS;
    } else if (status_ == PROCESS) {
      // Spawn a new CallData instance to serve new clients while we process
      // the one for this CallData. The instance will deallocate itself as
      // part of its FINISH state.
      new PitayaGrpcCall(_handlerFunc, service_, cq_);

      // The actual processing.
      *reply_ = _handlerFunc(request_);

      // And we are done! Let the gRPC runtime know we've finished, using the
      // memory address of this instance as the uniquely identifying tag for
      // the event.
      responder_.Finish(*reply_, Status::OK, this);
      status_ = FINISH;
    } else {
      GPR_ASSERT(status_ == FINISH);
      // Once in the FINISH state, deallocate ourselves (CallData).
      delete this;
    }
  }

  private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    protos::Pitaya::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    protos::Request request_;
    // What we send back to the client.
    protos::Response* reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<protos::Response> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
    
    pitaya::RpcHandlerFunc _handlerFunc;
};


//class PitayaGrpcImpl final : public protos::Pitaya::Service
//{
//public:
//    PitayaGrpcImpl(pitaya::RpcHandlerFunc handlerFunc, const char* loggerName = nullptr)
//        : _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag)
//                          : spdlog::stdout_color_mt(kLogTag))
//        , _handlerFunc(std::move(handlerFunc))
//    {}
//
//    ~PitayaGrpcImpl() { spdlog::drop(kLogTag); }
//
//    grpc::Status AsyncCall(grpc::ServerContext* context,
//                      const protos::Request* req,
//                      protos::Response* res) override
//    {
//        *res = _handlerFunc(*req);
//        return grpc::Status::OK;
//    }
//
//private:
//    static constexpr const char* kLogTag = "grpc_server_impl";
//
//    std::shared_ptr<spdlog::logger> _log;
//    pitaya::RpcHandlerFunc _handlerFunc;
//};

namespace pitaya {

GrpcServer::GrpcServer(GrpcConfig config, RpcHandlerFunc handler, const char* loggerName)
    : RpcServer(handler)
    , _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _config(std::move(config))
{
    const auto address = _config.host + ":" + std::to_string(_config.port);

    _log->debug("Creating gRPC server at address {}", address);

    ServerBuilder builder;
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);

    cq_ = builder.AddCompletionQueue();

    _grpcServer = std::unique_ptr<::grpc::Server>(builder.BuildAndStart());

    if (_grpcServer) {
        _log->info("gRPC server started at: {}", address);
    } else {
        _log->error("Failed to start gRPC server at address {}", address);
    }
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

void
GrpcServer::ThreadStart()
{
    _log->info("Starting grpc rpc server thread");

    new PitayaGrpcCall(_handlerFunc, &service_, cq_.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
        // Block waiting to read the next event from the completion queue. The
        // event is uniquely identified by its tag, which in this case is the
        // memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or cq_ is shutting down.
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<PitayaGrpcCall*>(tag)->Proceed();
    }
} 
}
// namespace pitaya
