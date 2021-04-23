#include <etcd/v3/AsyncLeaseRevokeAction.hpp>
#include <etcd/v3/Transaction.hpp>

using etcdserverpb::LeaseRevokeRequest;
using grpc::ClientAsyncResponseReader;
using etcdserverpb::LeaseRevokeResponse;

etcdv3::AsyncLeaseRevokeAction::AsyncLeaseRevokeAction(etcdv3::ActionParameters param)
  : etcdv3::Action(std::move(param))
{
    LeaseRevokeRequest lease_revoke_request;
    lease_revoke_request.set_id(parameters.lease_id);

    response_reader = parameters.lease_stub->AsyncLeaseRevoke(&context, lease_revoke_request, &cq_);
    response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseRevokeResponse etcdv3::AsyncLeaseRevokeAction::ParseResponse()
{
    if (!status.ok()) {
        return AsyncLeaseRevokeResponse(status.error_code(), status.error_message());
    }

    return AsyncLeaseRevokeResponse(reply);
}
