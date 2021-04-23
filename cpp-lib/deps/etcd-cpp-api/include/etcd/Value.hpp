#ifndef __ETCD_VECTOR_HPP__
#define __ETCD_VECTOR_HPP__

#include <string>
#include <vector>

#include <pplx/pplxtasks.h>

#include <etcd/v3/KeyValue.hpp>


namespace etcd
{
  /**
   * Represents a value object received from the etcd server
   */
  class Value
  {
  public:
    std::string key;
    bool        is_dir = false;
    std::string value;
    int64_t     created_revision = 0;
    int64_t     modified_revision = 0;
    int         ttl = 0;
    int64_t     lease_id = 0;

  protected:
    friend class Response;
    Value() = default;
    Value(etcdv3::KeyValue const & kvs) = delete;
    Value(etcdv3::KeyValue && kvs);
  };

  using Values = std::vector<Value>;
}

#endif
