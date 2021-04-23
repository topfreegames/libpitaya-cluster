#ifndef __ASYNC_LEASEGRANTRESPONSE_HPP__
#define __ASYNC_LEASEGRANTRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/V3Response.hpp>


using etcdserverpb::LeaseGrantResponse;

namespace etcdv3
{
  class AsyncLeaseGrantResponse : public etcdv3::V3Response
  {
  public:
    using V3Response::V3Response;
    AsyncLeaseGrantResponse(LeaseGrantResponse const & resp);
  };
}

#endif
