#include <etcd/v3/AsyncUpdateAction.hpp>
#include <etcd/v3/AsyncRangeResponse.hpp>
#include <etcd/v3/action_constants.hpp>
#include <etcd/v3/Transaction.hpp>

using etcdserverpb::Compare;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncUpdateAction::AsyncUpdateAction(etcdv3::ActionParameters param)
  : etcdv3::Action(std::move(param))
{
  etcdv3::Transaction transaction(parameters.key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_GREATER,
                           Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_compare_and_swap_sequence(parameters.value, parameters.lease_id);

  response_reader = parameters.kv_stub->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncUpdateAction::ParseResponse()
{
  if (!status.ok())
  {
    return AsyncTxnResponse(status.error_code(), status.error_message());
  }

  if (reply.succeeded())
  {
    return AsyncTxnResponse(reply, parameters.withPrefix, etcdv3::UPDATE_ACTION);
  }

  return AsyncTxnResponse(etcdv3::V3StatusCode::KEY_NOT_FOUND, "Key not found");
}
