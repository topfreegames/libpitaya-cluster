#include <etcd/v3/AsyncWatchAction.hpp>
#include <etcd/v3/action_constants.hpp>


using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;
using etcdserverpb::WatchCreateRequest;
using etcdserverpb::WatchCancelRequest;

enum class Type {
    Create = 1,
    Write = 2,
    Read = 3,
    Finish = 4,
    WritesDone = 5,
    WriteWatchCancel = 6,
    ReadWatchCancel = 7,
    CancelWatch = 8,
};

etcdv3::AsyncWatchAction::AsyncWatchAction(etcdv3::ActionParameters param)
  : etcdv3::Action(std::move(param))
  , revision(parameters.revision)
  , isCancelled(false)
  , stream(parameters.watch_stub->AsyncWatch(&context, &cq_, reinterpret_cast<void*>(Type::Create)))
{
  WatchRequest watch_req;
  {
    WatchCreateRequest watch_create_req;
    watch_create_req.set_key(parameters.key);
    // TODO: set_prev_kv - as param from outside
    watch_create_req.set_prev_kv(true);
    watch_create_req.set_start_revision(parameters.revision);
    //watch_create_req.set_progress_notify(true);

    if(parameters.withPrefix)
    {
      std::string range_end(parameters.key);
      int ascii = static_cast<int>(range_end[range_end.length() - 1]);
      range_end.back() = ascii + 1;
      watch_create_req.set_range_end(std::move(range_end));
    }

    *(watch_req.mutable_create_request()) = std::move(watch_create_req);
  }

  void * got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == reinterpret_cast<void*>(Type::Create))
  {
    stream->Write(watch_req, reinterpret_cast<void*>(Type::Write));
    ok = false;
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == reinterpret_cast<void*>(Type::Write))
    {
      stream->Read(&response, reinterpret_cast<void*>(Type::Read));
      ok = false;
      if (cq_.Next(&got_tag, &ok) && ok && got_tag == reinterpret_cast<void*>(Type::Read))
      {
        _watchId = response.watch_id();
        // parse create response
        storeLastRevision();
        if (response.compact_revision() > 0)
        {
          throw async_watch_error("watch has not been created on etcd server due to trying to watch compacted revision");
        }
        if (response.canceled())
        {
          throw async_watch_error("watch has been cancelled when creating on etcd server");
        }
        if (!response.created())
        {
          throw async_watch_error("watch had not been created on etcd server");
        }
        stream->Read(&response, reinterpret_cast<void*>(Type::Read));
      }
      else
      {
        cq_.Shutdown();
        throw async_watch_error("no answer from etcd server");
      }
    }
    else
    {
      cq_.Shutdown();
      throw async_watch_error("cannot send watch create request to etcd server");
    }
  }
  else
  {
    cq_.Shutdown();
    throw async_watch_error("cannot call gRPC method for watch");
  }
}

// TODO: throw in case of !ok
void etcdv3::AsyncWatchAction::waitForResponse() 
{
  // NOTE(leo): Where is this function called?
  void * got_tag;
  bool ok = false;

  while (true) {
    if (cq_.Next(&got_tag, &ok)) {
      if (!ok) {
        cq_.Shutdown();
        continue;
      }

      auto tagType = static_cast<Type>(reinterpret_cast<size_t>(got_tag));

      switch (tagType) {
        case Type::Read: {
          storeLastRevision();

          if (response.events_size()) {
            stream->WritesDone(reinterpret_cast<void*>(Type::WritesDone));
          } else {
            stream->Read(&response, reinterpret_cast<void*>(Type::Read));
          }
          break;
        }
        default: {
          context.TryCancel();
          cq_.Shutdown();
          break;
        }
      }
    } else {
      // Completion queue is shut down and drained
      break;
    }
  }
}

// TODO: maybe rework with WatchCancelResponse
void etcdv3::AsyncWatchAction::cancelWatch()
{
  if (!isCancelled)
  {
//    check if cq_ is ok
    isCancelled = true;
    void * got_tag;
    bool ok = false;
    gpr_timespec deadline;
    deadline.clock_type = GPR_TIMESPAN;
    deadline.tv_sec = 0;
    deadline.tv_nsec = 10000000;

    if (cq_.AsyncNext(&got_tag, &ok, deadline) != CompletionQueue::SHUTDOWN)
    {
      WatchRequest watch_req;
      {
        WatchCancelRequest watch_cancel_req;
        watch_cancel_req.set_watch_id(_watchId);
        *(watch_req.mutable_cancel_request()) = std::move(watch_cancel_req);
      }

      stream->Write(watch_req, reinterpret_cast<void*>(Type::WriteWatchCancel));
    }
  }
}

int64_t etcdv3::AsyncWatchAction::lastRevision() const
{
  return revision;
}

void etcdv3::AsyncWatchAction::storeLastRevision()
{
  if (response.has_header() && response.header().revision() > 0)
  {
    revision = response.header().revision();
  }
}

// TODO: throw in case of !ok
void etcdv3::AsyncWatchAction::waitForResponse(watch_callback const & callback)
{
  void * got_tag;
  bool ok = false;

  while (true) {
    if (cq_.Next(&got_tag, &ok)) {
      if (!ok) {
        cq_.Shutdown();
        continue;
      }

      auto tagType = static_cast<Type>(reinterpret_cast<size_t>(got_tag));

      switch (tagType) {
        case Type::Read: {
          storeLastRevision();

          if (response.events_size()) {
            try {
              callback(etcd::Response(ParseResponse()));
            } catch (...) {}
          }

          stream->Read(&response, reinterpret_cast<void*>(Type::Read));
          break;
        }
        case Type::WriteWatchCancel: {
          stream->WritesDone(reinterpret_cast<void*>(Type::WritesDone));
          break;
        }
        case Type::WritesDone: {
          context.TryCancel();
          cq_.Shutdown();
          break;
        }
        default: {
          cq_.Shutdown();
          break;
        }
      }
    } else {
      // Completion queue is shut down and drained
      break;
    }
  }
}

etcdv3::AsyncWatchResponse etcdv3::AsyncWatchAction::ParseResponse()
{
  if (!status.ok())
  {
    return AsyncWatchResponse(status.error_code(), status.error_message());
  }

  return AsyncWatchResponse(response);
}
