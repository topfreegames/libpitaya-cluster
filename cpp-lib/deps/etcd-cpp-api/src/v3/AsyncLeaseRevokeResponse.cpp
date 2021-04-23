#include "etcd/v3/AsyncLeaseRevokeResponse.hpp"


etcdv3::AsyncLeaseRevokeResponse::AsyncLeaseRevokeResponse(etcdserverpb::LeaseRevokeResponse const & resp)
{
    revision = resp.header().revision();
    status.etcd_error_code = V3StatusCode::OK;
}
