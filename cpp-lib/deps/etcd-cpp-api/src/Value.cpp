#include <iomanip>
#include <etcd/Value.hpp>
#include <etcd/v3/KeyValue.hpp>


etcd::Value::Value(etcdv3::KeyValue && kv)
  : key(std::move(*(kv.kvs.mutable_key())))
  , value(std::move(*(kv.kvs.mutable_value())))
  , created_revision(kv.kvs.create_revision())
  , modified_revision(kv.kvs.mod_revision())
  , ttl(kv.get_ttl())
  , lease_id(kv.kvs.lease())
{}
