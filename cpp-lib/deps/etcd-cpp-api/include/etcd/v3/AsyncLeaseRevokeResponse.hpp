#ifndef __ASYNC_LEASEREVOKERESPONSE_HPP__
#define __ASYNC_LEASEREVOKERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/V3Response.hpp>


using etcdserverpb::LeaseRevokeResponse;

namespace etcdv3
{
  class AsyncLeaseRevokeResponse : public etcdv3::V3Response
  {
  public:
    using V3Response::V3Response;
    AsyncLeaseRevokeResponse(LeaseRevokeResponse const & resp);
  };
}

#endif
