#ifndef __ASYNC_WATCH_HPP__
#define __ASYNC_WATCH_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/proto/rpc.pb.h>
#include <etcd/v3/V3Response.hpp>


using etcdserverpb::WatchRequest;//
using etcdserverpb::WatchResponse;
using etcdserverpb::KV;//

namespace etcdv3
{
  class AsyncWatchResponse : public etcdv3::V3Response
  {
  public:
    using V3Response::V3Response;
    AsyncWatchResponse(WatchResponse const & reply);
  };
}

#endif

