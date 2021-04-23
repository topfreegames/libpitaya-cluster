#include <etcd/Utils.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/v3/AsyncGetAction.hpp>


etcd::Watcher::Watcher(
    std::string const & address,
    std::string const & key,
    std::function<void(Response)> callback,
    pplx::task_options const & task_options)
  : Watcher::Watcher(etcd::utils::createChannel(address), key, true, 0, callback, task_options)
{}

etcd::Watcher::Watcher(
    std::shared_ptr<grpc::Channel> const & channel,
    std::string const & key,
    std::function<void(Response)> callback,
    pplx::task_options const & task_options)
  : Watcher::Watcher(channel, key, true, 0, callback, task_options)
{}

etcd::Watcher::Watcher(
    std::string const & address,
    std::string const & key,
    bool const recursive,
    int const fromRevision,
    std::function<void(Response)> callback,
    pplx::task_options const & task_options)
  : Watcher::Watcher(etcd::utils::createChannel(address), key, recursive, fromRevision, callback, task_options)
{}

etcd::Watcher::Watcher(
    std::shared_ptr<grpc::Channel> const & channel,
    std::string const & key,
    bool const recursive,
    int const fromRevision,
    std::function<void(Response)> callback,
    pplx::task_options const & task_options)
  : channel(channel)
  , watchServiceStub(Watch::NewStub(channel))
  , task_options(task_options)
  , callback(callback)
  , isCancelled(false)
{
  //auto fromRevisionExplicit = fromRevision;
  //if (fromRevisionExplicit == 0)
  //{
    //const auto stub = etcdserverpb::KV::NewStub(channel);
    //etcdv3::ActionParameters get_params;
    //get_params.key.assign(key);
    //get_params.withPrefix = recursive;
    //get_params.kv_stub = stub.get();
    //auto get_call = std::make_shared<etcdv3::AsyncGetAction>(std::move(get_params));
    //get_call->waitForResponse();
    //fromRevisionExplicit = get_call->ParseResponse().revision;
    //if (fromRevisionExplicit == 0)
    //{
      //throw watch_error("cannot acquire current etcd key-value store revision");
    //}
  //}
  watch_action_parameters.key = key;
  watch_action_parameters.withPrefix = recursive;
  watch_action_parameters.watch_stub = watchServiceStub.get();
  // if fromRevision is 0, interested revision for user is current + 1
  watch_action_parameters.revision = 0;
  try
  {
    doWatch();
  }
  catch (etcdv3::async_watch_error const & e)
  {
    throw watch_error(e.what());
  }

}

void etcd::Watcher::cancel()
{
  if (isCancelled)
  {
    return;
  }
  if (call)
  {
    call->cancelWatch();
    currentTask.wait();
    call.reset();
  }
  isCancelled = true;
}

bool etcd::Watcher::cancelled() const
{
  return isCancelled;
}

etcd::Watcher::~Watcher()
{
  try
  {
    cancel();
  } catch (...)
  {}
}

void etcd::Watcher::doWatch()
{
  try
  {
    if (call)
    {
      call->cancelWatch();
    }
    currentTask.wait();
  }
  catch (...)
  {}
  if (call)
  {
    // in case of watch recreate, continue from revision next after last received revision
    // lastRevision always returns explicit revision number (not 0)
    watch_action_parameters.revision = call->lastRevision() + 1;
  }
  call.reset(new etcdv3::AsyncWatchAction(watch_action_parameters));
  currentTask = pplx::task<void>([this]()
  {
    call->waitForResponse(callback);
  }, task_options);
}
