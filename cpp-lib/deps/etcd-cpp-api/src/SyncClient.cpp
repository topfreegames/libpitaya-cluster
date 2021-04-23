#include <etcd/SyncClient.hpp>
#include <etcd/v3/V3Status.hpp>

#define CHECK_EXCEPTIONS(cmd)                   \
  try                                           \
{                                             \
  return cmd;                                 \
  }                                             \
  catch (std::exception const & ex)             \
{                                             \
  return etcd::Response(etcd::StatusCode::USER_DEFINED_ERROR, ex.what());      \
  }

etcd::SyncClient::SyncClient(std::string const & address)
  : client(address)
{
}

etcd::Response etcd::SyncClient::get(std::string const & key)
{
  CHECK_EXCEPTIONS(client.get(key).get());
}

etcd::Response etcd::SyncClient::set(std::string const & key, std::string const & value, int const ttl)
{
  CHECK_EXCEPTIONS(client.set(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::set(std::string const & key, std::string const & value, int64_t const lease_id)
{
  CHECK_EXCEPTIONS(client.set(key, value, lease_id).get());
}

etcd::Response etcd::SyncClient::add(std::string const & key, std::string const & value, int const ttl)
{
  CHECK_EXCEPTIONS(client.add(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::add(std::string const & key, std::string const & value, int64_t const lease_id)
{
  CHECK_EXCEPTIONS(client.add(key, value, lease_id).get());
}

etcd::Response etcd::SyncClient::modify(std::string const & key, std::string const & value, int const ttl)
{
  CHECK_EXCEPTIONS(client.modify(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::modify(std::string const & key, std::string const & value, int64_t const lease_id)
{
  CHECK_EXCEPTIONS(client.modify(key, value, lease_id).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int const ttl)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_value, ttl).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t const lease_id)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_value, lease_id).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int const ttl)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_revision, ttl).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, int64_t const old_revision, int64_t const lease_id)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_revision, lease_id).get());
}

etcd::Response etcd::SyncClient::rm(std::string const & key)
{
  CHECK_EXCEPTIONS(client.rm(key).get());
}

etcd::Response etcd::SyncClient::rm_if(std::string const & key, std::string const & old_value)
{
  CHECK_EXCEPTIONS(client.rm_if(key, old_value).get());
}

etcd::Response etcd::SyncClient::rm_if(std::string const & key, int64_t const old_revision)
{
  CHECK_EXCEPTIONS(client.rm_if(key, old_revision).get());
}


etcd::Response etcd::SyncClient::rmdir(std::string const & key, bool const recursive)
{
  CHECK_EXCEPTIONS(client.rmdir(key, recursive).get());
}

etcd::Response etcd::SyncClient::ls(std::string const & key, bool const keysOnly)
{
  CHECK_EXCEPTIONS(client.ls(key, keysOnly).get());
}

etcd::Response etcd::SyncClient::leasegrant(int const ttl)
{
  CHECK_EXCEPTIONS(client.leasegrant(ttl).get());
}

// etcd::Response etcd::SyncClient::watch(std::string const & key, bool recursive)
// {
//   web::http::uri_builder uri("/v2/keys" + key);
//   uri.append_query("wait=true");
//   if (recursive)
//     uri.append_query("recursive=true");
//   return send_get_request(uri);
// }

// etcd::Response etcd::SyncClient::watch(std::string const & key, int fromIndex, bool recursive)
// {
//   web::http::uri_builder uri("/v2/keys" + key);
//   uri.append_query("wait=true");
//   uri.append_query("waitIndex", fromIndex);
//   if (recursive)
//     uri.append_query("recursive=true");
//   return send_get_request(uri);
// }
