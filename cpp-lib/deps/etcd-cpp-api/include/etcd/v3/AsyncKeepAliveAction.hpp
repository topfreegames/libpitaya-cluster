#ifndef __ASYNC_KEEPALIVEACTION_HPP__
#define __ASYNC_KEEPALIVEACTION_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/Action.hpp>
#include <etcd/v3/AsyncKeepAliveResponse.hpp>

using grpc::ClientAsyncReaderWriter;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseKeepAliveResponse;

namespace etcdv3
{
    class AsyncKeepAliveAction : public etcdv3::Action
    {
    public:
        AsyncKeepAliveAction(etcdv3::ActionParameters param);
        AsyncKeepAliveResponse ParseResponse();

        void waitForResponse();
        void setLeaseId(int64_t lease_id);

    private:
        LeaseKeepAliveResponse _response;
        std::unique_ptr<ClientAsyncReaderWriter<LeaseKeepAliveRequest,LeaseKeepAliveResponse>> _stream;
    };
}

#endif // __ASYNC_KEEPALIVEACTION_HPP__
