#ifndef __ASYNC_RANGERESPONSE_HPP__
#define __ASYNC_RANGERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/V3Response.hpp>


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncRangeResponse : public etcdv3::V3Response
  {
  public:
    using V3Response::V3Response;
    AsyncRangeResponse(RangeResponse const & resp, bool const prefix = false);
  };
}

#endif
