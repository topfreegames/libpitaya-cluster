#include "etcd/v3/AsyncKeepAliveResponse.hpp"

etcdv3::AsyncKeepAliveResponse::AsyncKeepAliveResponse(LeaseKeepAliveResponse const &resp)
{
    revision = resp.header().revision();
    status.etcd_error_code = status.etcd_error_message.empty() ? V3StatusCode::OK : V3StatusCode::OTHER_ERROR;
    value.kvs.set_lease(resp.id());
    value.set_ttl(resp.ttl());
}
