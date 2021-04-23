#ifndef __ASYNC_KEEPALIVERESPONSE_HPP__
#define __ASYNC_KEEPALIVERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/V3Response.hpp>

using etcdserverpb::LeaseKeepAliveResponse;

namespace etcdv3
{
    class AsyncKeepAliveResponse : public etcdv3::V3Response
    {
    public:
        using V3Response::V3Response;
        AsyncKeepAliveResponse(LeaseKeepAliveResponse const & resp);
    };
}

#endif // __ASYNC_KEEPALIVERESPONSE_HPP__
