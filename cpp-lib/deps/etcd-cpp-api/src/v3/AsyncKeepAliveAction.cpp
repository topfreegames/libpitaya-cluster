#include "etcd/v3/AsyncKeepAliveAction.hpp"
#include <etcd/v3/Transaction.hpp>
#include <chrono>

using etcdserverpb::LeaseGrantRequest;

enum class Type {
    Connect = 1,
    Write = 2,
    Read = 3,
    Finish = 4,
};

etcdv3::AsyncKeepAliveAction::AsyncKeepAliveAction(ActionParameters param)
: etcdv3::Action(std::move(param))
, _stream(parameters.lease_stub->AsyncLeaseKeepAlive(&context, &cq_, reinterpret_cast<void*>(Type::Connect)))
{
    LeaseKeepAliveRequest keep_alive_req;
    keep_alive_req.set_id(parameters.lease_id);

    void *got_tag;
    bool ok = false;

    if (!cq_.Next(&got_tag, &ok)) {
        throw std::runtime_error("Failed to create lease keep alive stream");
    }

    if (!ok) {
        cq_.Shutdown();
        throw std::runtime_error("Completion queue not OK!");
    }

    Type tag = static_cast<Type>(reinterpret_cast<size_t>(got_tag));
    GPR_ASSERT(tag == Type::Connect);
}

void etcdv3::AsyncKeepAliveAction::waitForResponse()
{
    LeaseKeepAliveRequest keep_alive_req;
    keep_alive_req.set_id(parameters.lease_id);

    void* got_tag = nullptr;
    bool ok = false;

    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(5000);


    _stream->Write(keep_alive_req, reinterpret_cast<void*>(Type::Write));
    // wait write finish
    switch (cq_.AsyncNext(&got_tag, &ok, deadline)) {
      case CompletionQueue::NextStatus::TIMEOUT: {
        status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "gRPC timeout during keep alive write");
        break;
      }
      case CompletionQueue::NextStatus::SHUTDOWN: {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "gRPC already shutdown during keep alive write");
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        if (!ok || got_tag != reinterpret_cast<void*>(Type::Write)) {
          return;
        }
      }
    }
    if (!status.ok()) {
      return;
    }

    deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(5000);
    _stream->Read(&_response, reinterpret_cast<void*>(Type::Read));
    // wait read finish
    switch (cq_.AsyncNext(&got_tag, &ok, deadline)) {
      case CompletionQueue::NextStatus::TIMEOUT: {
        status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "gRPC timeout during keep alive read");
        break;
      }
      case CompletionQueue::NextStatus::SHUTDOWN: {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "gRPC already shutdown during keep alive read");
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        if (ok && got_tag == reinterpret_cast<void*>(Type::Read)) {
          return;
        }
        break;
      }
    }

}

void etcdv3::AsyncKeepAliveAction::setLeaseId(int64_t lease_id)
{
    parameters.lease_id = lease_id;
}

etcdv3::AsyncKeepAliveResponse
etcdv3::AsyncKeepAliveAction::ParseResponse()
{
    if (!status.ok()) {
        return AsyncKeepAliveResponse(status.error_code(), status.error_message());
    }

    return AsyncKeepAliveResponse(_response);
}
