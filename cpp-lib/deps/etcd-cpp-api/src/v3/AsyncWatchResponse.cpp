#include <etcd/v3/AsyncWatchResponse.hpp>
#include <etcd/v3/action_constants.hpp>


etcdv3::AsyncWatchResponse::AsyncWatchResponse(WatchResponse const & reply)
{
  revision = reply.header().revision();

  for (int cnt = reply.events_size() - 1; cnt >= 0; cnt--)
  {
    auto event = reply.events(cnt);

    switch (event.type())
    {
      case mvccpb::Event::EventType::Event_EventType_PUT:
        action = (event.kv().version() == 1) ? etcdv3::CREATE_ACTION : etcdv3::SET_ACTION;
        value.kvs.CopyFrom(event.kv());
        break;
      case mvccpb::Event::EventType::Event_EventType_DELETE:
        action = etcdv3::DELETE_ACTION;
        value.kvs.CopyFrom(event.kv());
        break;
      default:
        break;
    }

    if (event.has_prev_kv())
    {
      prev_value.kvs.CopyFrom(event.prev_kv());
    }

    // TODO: maybe change this logic, adding values too
    // just store the first occurence of the key in values (with latest revision).
    // this is done so tas client will not need to change their behaviour.
    // break immediately
    break;
  }
}
