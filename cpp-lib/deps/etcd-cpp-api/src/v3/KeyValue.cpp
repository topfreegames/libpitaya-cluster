#include <etcd/v3/KeyValue.hpp>

void etcdv3::KeyValue::set_ttl(int const ttl)
{
  this->ttl = ttl;
}

int etcdv3::KeyValue::get_ttl() const
{
  return ttl;
}
