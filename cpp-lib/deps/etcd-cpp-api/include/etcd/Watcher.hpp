#ifndef __ETCD_WATCHER_HPP__
#define __ETCD_WATCHER_HPP__

#include <string>
#include <etcd/Response.hpp>
#include <etcd/v3/AsyncWatchAction.hpp>

#include <grpc++/grpc++.h>

using etcdserverpb::Watch;
using grpc::Channel;

namespace etcd
{

  class watch_error
      : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
    using std::runtime_error::what;
  };

  class Watcher
  {
  public:
    Watcher(
        std::string const & address,
        std::string const & key,
        std::function<void(Response)> callback,
        pplx::task_options const & task_options = pplx::task_options());
    Watcher(
        std::shared_ptr<grpc::Channel> const & channel,
        std::string const & key,
        std::function<void(Response)> callback,
        pplx::task_options const & task_options = pplx::task_options());
    Watcher(
        std::string const & address,
        std::string const & key,
        bool const recursive,
        int const fromRevision,
        std::function<void(Response)> callback,
        pplx::task_options const & task_options = pplx::task_options());
    Watcher(
        std::shared_ptr<Channel> const & channel,
        std::string const & key,
        bool const recursive,
        int const fromRevision,
        std::function<void(Response)> callback,
        pplx::task_options const & task_options = pplx::task_options());
    void cancel();
    bool cancelled() const;
    ~Watcher();

  protected:
    void doWatch();

    const std::shared_ptr<Channel> channel;
    const std::unique_ptr<Watch::Stub> watchServiceStub;
    const pplx::task_options task_options;
    etcdv3::ActionParameters watch_action_parameters;
    std::function<void(Response)> callback;
    bool isCancelled;
    pplx::task<void> currentTask;
    std::unique_ptr<etcdv3::AsyncWatchAction> call;
  };
} // namespace etcd

#endif
