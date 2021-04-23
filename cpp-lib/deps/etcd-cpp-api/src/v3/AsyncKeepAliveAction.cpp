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

    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    CompletionQueue::NextStatus nextStatus = cq_.AsyncNext(&got_tag, &ok, deadline);

    if (nextStatus == CompletionQueue::SHUTDOWN) {
        return;
    }

    _stream->Write(keep_alive_req, reinterpret_cast<void*>(Type::Write));

    for (;;) {
        if (!cq_.Next(&got_tag, &ok)) {
            break;
        }

        if (!ok) {
            _stream->Finish(&status, reinterpret_cast<void*>(Type::Finish));
            cq_.Shutdown();
            continue;
        }

        switch (static_cast<Type>(reinterpret_cast<size_t>(got_tag))) {
            case Type::Read:
                return;
            case Type::Write:
                _stream->Read(&_response, reinterpret_cast<void*>(Type::Read));
                break;
            case Type::Finish:
                return;
            default:
                GPR_ASSERT(false);
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
