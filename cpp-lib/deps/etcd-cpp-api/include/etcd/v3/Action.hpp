#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/proto/rpc.grpc.pb.h>

using grpc::ClientContext;
using grpc::CompletionQueue;

using etcdserverpb::KV;
using etcdserverpb::Watch;
using etcdserverpb::Lease;

namespace etcdv3
{
  enum Atomicity_Type
  {
    PREV_INDEX = 0,
    PREV_VALUE = 1
  };

  struct ActionParameters
  {
    ActionParameters() = default;
    ActionParameters(ActionParameters &&) = default;
    ActionParameters(ActionParameters const &) = default;
    bool withPrefix = false;
    bool keysOnly = false;
    int64_t revision = 0;
    int64_t old_revision = 0;
    int64_t lease_id = 0;
    int ttl = 0;
    std::string key;
    std::string value;
    std::string old_value;
    KV::Stub* kv_stub = nullptr;
    Watch::Stub* watch_stub = nullptr;
    Lease::Stub* lease_stub = nullptr;
  };

  class Action
  {
  public:
    Action() = default;
    Action(ActionParameters params);
    void waitForResponse();
  protected:
    grpc::Status status;
    ClientContext context;
    CompletionQueue cq_;
    ActionParameters parameters;
  };
} // namespace etcdv3
#endif
