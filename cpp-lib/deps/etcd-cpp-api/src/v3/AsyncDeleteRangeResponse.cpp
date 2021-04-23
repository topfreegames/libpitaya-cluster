#include <etcd/v3/AsyncDeleteRangeResponse.hpp>
#include <etcd/v3/action_constants.hpp>


etcdv3::AsyncDeleteRangeResponse::AsyncDeleteRangeResponse(
    etcdserverpb::DeleteRangeResponse const & resp,
    bool const prefix)
{
  revision = resp.header().revision();
  if (resp.prev_kvs_size() == 0)
  {
    status.etcd_error_code = etcdv3::V3StatusCode::KEY_NOT_FOUND;
    status.etcd_error_message = "Key not found";
  }
  else
  {
    action = etcdv3::DELETE_ACTION;
    //get all previous values
    for (int cnt = 0; cnt < resp.prev_kvs_size(); cnt++)
    {
      etcdv3::KeyValue kv;
      kv.kvs.CopyFrom(resp.prev_kvs(cnt));
      values.push_back(kv);
    }
    if (!prefix)
    {
      prev_value = values[0];
      value = values[0];
      value.kvs.clear_value();
      values.clear();
    }
  }
}
