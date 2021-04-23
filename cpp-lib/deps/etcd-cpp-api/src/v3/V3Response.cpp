#include <etcd/v3/V3Response.hpp>


etcdv3::V3Response::V3Response(V3StatusCode const etcd_error_code, std::string etcd_error_message)
  : status(etcd_error_code, std::move(etcd_error_message))
{}

etcdv3::V3Response::V3Response(grpc::StatusCode const grpc_error_code, grpc::string grpc_error_message)
  : status(grpc_error_code, std::move(grpc_error_message))
{}

bool etcdv3::V3Response::has_values() const
{
  return values.size() > 0;
}
