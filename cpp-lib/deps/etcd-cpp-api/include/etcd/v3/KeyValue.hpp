#ifndef __V3_ETCDV3KEYVALUE_HPP__
#define __V3_ETCDV3KEYVALUE_HPP__

#include <etcd/v3/proto/kv.pb.h>


namespace etcdv3
{
  class KeyValue
  {
  public:
    KeyValue() = default;
    mvccpb::KeyValue kvs;
    void set_ttl(int const ttl);
    int get_ttl() const;
  private:
    int ttl = 0;
  };
}
#endif
