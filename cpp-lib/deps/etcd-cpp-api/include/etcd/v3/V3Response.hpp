#ifndef __V3_RESPONSE_HPP__
#define __V3_RESPONSE_HPP__

#include <grpc++/grpc++.h>
#include <etcd/v3/KeyValue.hpp>
#include <etcd/v3/V3Status.hpp>

namespace etcdv3
{
  struct V3Response
  {
    int64_t revision = 0;
    etcdv3::V3Status status;
    std::string action;
    etcdv3::KeyValue value;
    etcdv3::KeyValue prev_value;
    std::vector<etcdv3::KeyValue> values;
    std::vector<etcdv3::KeyValue> prev_values;

    V3Response() = default;
    V3Response(etcdv3::V3StatusCode const etcd_error_code, std::string etcd_error_message);
    V3Response(grpc::StatusCode const grpc_error_code, grpc::string grpc_error_message);
    V3Response(V3Response const &) = delete;
    V3Response & operator =(V3Response const &) = delete;
    V3Response(V3Response &&) = default;
    V3Response & operator =(V3Response &&) = default;

    bool has_values() const;
  };
}

#endif
