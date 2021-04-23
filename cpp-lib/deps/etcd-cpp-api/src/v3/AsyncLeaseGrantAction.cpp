#include <etcd/v3/AsyncLeaseGrantAction.hpp>
#include <etcd/v3/Transaction.hpp>


using etcdserverpb::LeaseGrantRequest;


etcdv3::AsyncLeaseGrantAction::AsyncLeaseGrantAction(etcdv3::ActionParameters param)
  : etcdv3::Action(std::move(param))
{
  etcdv3::Transaction transaction;
  transaction.setup_lease_grant_operation(parameters.ttl);

  response_reader = parameters.lease_stub->AsyncLeaseGrant(&context, transaction.leasegrant_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
  
}

etcdv3::AsyncLeaseGrantResponse etcdv3::AsyncLeaseGrantAction::ParseResponse()
{
  if (!status.ok())
  {
    return AsyncLeaseGrantResponse(status.error_code(), status.error_message());
  }

  return AsyncLeaseGrantResponse(reply);
}
