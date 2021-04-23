#include <etcd/v3/AsyncLeaseGrantResponse.hpp>


etcdv3::AsyncLeaseGrantResponse::AsyncLeaseGrantResponse(etcdserverpb::LeaseGrantResponse const & resp)
{
  revision = resp.header().revision();
  status.etcd_error_message = resp.error();
  status.etcd_error_code = status.etcd_error_message.empty() ? etcdv3::V3StatusCode::OK : etcdv3::V3StatusCode::OTHER_ERROR;
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
}
