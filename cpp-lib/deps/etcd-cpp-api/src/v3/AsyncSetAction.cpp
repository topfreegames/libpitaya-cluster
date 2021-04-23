#include <etcd/v3/AsyncSetAction.hpp>
#include <etcd/v3/action_constants.hpp>
#include <etcd/v3/Transaction.hpp>

using etcdserverpb::Compare;

etcdv3::AsyncSetAction::AsyncSetAction(etcdv3::ActionParameters param, bool const create)
  : etcdv3::Action(std::move(param))
  , isCreate(create)
{
  etcdv3::Transaction transaction(parameters.key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
                           Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_basic_create_sequence(parameters.key, parameters.value, parameters.lease_id);

  if(isCreate)
  {
    transaction.setup_basic_failure_operation(parameters.key);
  }
  else
  {
    transaction.setup_set_failure_operation(parameters.key, parameters.value, parameters.lease_id);
  }
  response_reader = parameters.kv_stub->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncSetAction::ParseResponse()
{
  if (!status.ok())
  {
    return AsyncTxnResponse(status.error_code(), status.error_message());
  }

  auto txn_resp = AsyncTxnResponse(reply, parameters.withPrefix, isCreate ? etcdv3::CREATE_ACTION : etcdv3::SET_ACTION);

  if (!reply.succeeded() && isCreate)
  {
    txn_resp.status.etcd_error_code = etcdv3::V3StatusCode::KEY_ALREADY_EXISTS;
    txn_resp.status.etcd_error_message = "Key already exists";
  }

  return txn_resp;
}
