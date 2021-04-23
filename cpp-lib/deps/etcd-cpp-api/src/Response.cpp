#include <etcd/Response.hpp>


etcd::Response::Response(etcd::StatusCode const etcd_error_code, std::string etcd_error_message)
  : status(etcd_error_code, std::move(etcd_error_message))
{}

etcd::Response::Response(etcd::Status && status)
  : status(std::move(status))
{}

etcd::Response::Response(etcd::Status const & status)
  : status(status)
{}

etcd::Response::Response(etcdv3::V3Response && response)
  : revision(response.revision)
  , status(std::move(response.status))
  , action(std::move(response.action))
  , prev_value(std::move(response.prev_value))
{
  if(response.has_values())
  {
    for(size_t index = 0; index < response.values.size(); index++)
    {
      keys.push_back(response.values[index].kvs.key());
      values.push_back(Value(std::move(response.values[index])));
    }
  }
  else
  {
    value = Value(std::move(response.value));
  }
}

bool etcd::Response::is_ok() const
{
  return status.is_ok();
}

bool etcd::Response::etcd_is_ok() const
{
  return status.etcd_is_ok();
}

bool etcd::Response::grpc_is_ok() const
{
  return status.grpc_is_ok();
}
