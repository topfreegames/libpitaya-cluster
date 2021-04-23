#ifndef __ASYNC_WATCHACTION_HPP__
#define __ASYNC_WATCHACTION_HPP__

#include <grpc++/grpc++.h>

#include <etcd/v3/proto/rpc.grpc.pb.h>
#include <etcd/v3/Action.hpp>
#include <etcd/v3/AsyncWatchResponse.hpp>
#include <etcd/Response.hpp>


using grpc::ClientAsyncReaderWriter;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;


namespace etcdv3
{

  class async_watch_error : std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
    using std::runtime_error::what;
  };

  using watch_callback = std::function<void(etcd::Response &&)>;

  class AsyncWatchAction : public etcdv3::Action
  {
  public:
    AsyncWatchAction(etcdv3::ActionParameters param);
    AsyncWatchResponse ParseResponse();
    void waitForResponse();
    void waitForResponse(watch_callback const & callback);
    void cancelWatch();
    int64_t lastRevision() const;

  private:
    WatchResponse response;
    int64_t _watchId;
    int64_t revision;
    bool isCancelled;
    std::unique_ptr<ClientAsyncReaderWriter<WatchRequest, WatchResponse>> stream;

    void storeLastRevision();
  };
}

#endif
