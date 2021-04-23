#ifndef STATUS_H
#define STATUS_H

#include<grpc++/grpc++.h>

namespace etcdv3
{
  enum class V3StatusCode
      : int
  {
    OK = 0,
    KEY_NOT_FOUND = 100,
    TEST_FAILED = 101,
    KEY_ALREADY_EXISTS = 105,
    KEY_READ_ONLY = 107,
    UNDERLYING_GRPC_ERROR = 900,
    OTHER_ERROR = 901,
    USER_DEFINED_ERROR = 902,
    UNKNOWN_ERROR = 903
  };

  /// Class representing both etcd and gRPC status codes and error messages
  struct V3Status
  {
    V3StatusCode etcd_error_code;
    grpc::StatusCode grpc_error_code;
    std::string etcd_error_message;
    std::string grpc_error_message;

    V3Status();
    V3Status(V3StatusCode const etcd_error_code, std::string etcd_error_message);
    V3Status(grpc::StatusCode const grpc_error_code, grpc::string grpc_error_message);
    V3Status(V3Status const &) = default;
    V3Status & operator =(V3Status const &) = default;
    V3Status(V3Status &&) = default;
    V3Status & operator =(V3Status &&) = default;

    bool is_ok() const;
    bool etcd_is_ok() const;
    bool grpc_is_ok() const;

    static V3StatusCode etcd_error_code_from_int(int const code);
  };
} // namespace etcd

#endif // STATUS_H
