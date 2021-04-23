#include <etcd/v3/AsyncTxnResponse.hpp>
#include <etcd/v3/AsyncRangeResponse.hpp>
#include <etcd/v3/AsyncDeleteRangeResponse.hpp>


using etcdserverpb::ResponseOp;


etcdv3::AsyncTxnResponse::AsyncTxnResponse(
    TxnResponse const & reply,
    bool const prefix,
    char const * const action)
{
  revision = reply.header().revision();
  this->action = action;
  for (int index = 0; index < reply.responses_size(); index++)
  {
    auto resp = reply.responses(index);

    switch (resp.response_case())
    {
      case ResponseOp::ResponseCase::kResponseRange:
      {
        AsyncRangeResponse response(*(resp.mutable_response_range()), prefix);
        status = std::move(response.status);
        values = std::move(response.values);
        value = std::move(response.value);
        break;
      }
      case ResponseOp::ResponseCase::kResponsePut:
      {
        auto put_resp = resp.response_put();
        if (put_resp.has_prev_kv()) {
          prev_value.kvs.CopyFrom(put_resp.prev_kv());
        }
        break;
      }
      case ResponseOp::ResponseCase::kResponseDeleteRange:
      {
        AsyncDeleteRangeResponse response(*(resp.mutable_response_delete_range()), prefix);
        status = std::move(response.status);
        prev_value.kvs = std::move(response.prev_value.kvs);
        values = std::move(response.values);
        value = std::move(response.value);
      }
      default:
        break;
    }
  }
}
