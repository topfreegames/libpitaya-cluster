// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: pitaya.proto

#include "pitaya.pb.h"
#include "pitaya.grpc.pb.h"

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace protos {

static const char* Pitaya_method_names[] = {
  "/protos.Pitaya/Call",
  "/protos.Pitaya/PushToUser",
  "/protos.Pitaya/SessionBindRemote",
  "/protos.Pitaya/KickUser",
};

std::unique_ptr< Pitaya::Stub> Pitaya::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< Pitaya::Stub> stub(new Pitaya::Stub(channel));
  return stub;
}

Pitaya::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Call_(Pitaya_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_PushToUser_(Pitaya_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_SessionBindRemote_(Pitaya_method_names[2], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_KickUser_(Pitaya_method_names[3], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status Pitaya::Stub::Call(::grpc::ClientContext* context, const ::protos::Request& request, ::protos::Response* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Call_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::AsyncCallRaw(::grpc::ClientContext* context, const ::protos::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_Call_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::PrepareAsyncCallRaw(::grpc::ClientContext* context, const ::protos::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_Call_, context, request, false);
}

::grpc::Status Pitaya::Stub::PushToUser(::grpc::ClientContext* context, const ::protos::Push& request, ::protos::Response* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_PushToUser_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::AsyncPushToUserRaw(::grpc::ClientContext* context, const ::protos::Push& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_PushToUser_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::PrepareAsyncPushToUserRaw(::grpc::ClientContext* context, const ::protos::Push& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_PushToUser_, context, request, false);
}

::grpc::Status Pitaya::Stub::SessionBindRemote(::grpc::ClientContext* context, const ::protos::BindMsg& request, ::protos::Response* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_SessionBindRemote_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::AsyncSessionBindRemoteRaw(::grpc::ClientContext* context, const ::protos::BindMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_SessionBindRemote_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::protos::Response>* Pitaya::Stub::PrepareAsyncSessionBindRemoteRaw(::grpc::ClientContext* context, const ::protos::BindMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::Response>::Create(channel_.get(), cq, rpcmethod_SessionBindRemote_, context, request, false);
}

::grpc::Status Pitaya::Stub::KickUser(::grpc::ClientContext* context, const ::protos::KickMsg& request, ::protos::KickAnswer* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_KickUser_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::protos::KickAnswer>* Pitaya::Stub::AsyncKickUserRaw(::grpc::ClientContext* context, const ::protos::KickMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::KickAnswer>::Create(channel_.get(), cq, rpcmethod_KickUser_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::protos::KickAnswer>* Pitaya::Stub::PrepareAsyncKickUserRaw(::grpc::ClientContext* context, const ::protos::KickMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::protos::KickAnswer>::Create(channel_.get(), cq, rpcmethod_KickUser_, context, request, false);
}

Pitaya::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Pitaya_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Pitaya::Service, ::protos::Request, ::protos::Response>(
          std::mem_fn(&Pitaya::Service::Call), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Pitaya_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Pitaya::Service, ::protos::Push, ::protos::Response>(
          std::mem_fn(&Pitaya::Service::PushToUser), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Pitaya_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Pitaya::Service, ::protos::BindMsg, ::protos::Response>(
          std::mem_fn(&Pitaya::Service::SessionBindRemote), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Pitaya_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Pitaya::Service, ::protos::KickMsg, ::protos::KickAnswer>(
          std::mem_fn(&Pitaya::Service::KickUser), this)));
}

Pitaya::Service::~Service() {
}

::grpc::Status Pitaya::Service::Call(::grpc::ServerContext* context, const ::protos::Request* request, ::protos::Response* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Pitaya::Service::PushToUser(::grpc::ServerContext* context, const ::protos::Push* request, ::protos::Response* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Pitaya::Service::SessionBindRemote(::grpc::ServerContext* context, const ::protos::BindMsg* request, ::protos::Response* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Pitaya::Service::KickUser(::grpc::ServerContext* context, const ::protos::KickMsg* request, ::protos::KickAnswer* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace protos

