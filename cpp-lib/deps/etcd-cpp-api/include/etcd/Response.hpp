#ifndef __ETCD_RESPONSE_HPP__
#define __ETCD_RESPONSE_HPP__

#include <string>
#include <vector>

#include <etcd/Value.hpp>
#include <etcd/v3/V3Response.hpp>


namespace etcd
{
  using StatusCode = etcdv3::V3StatusCode;
  using Status = etcdv3::V3Status;
  typedef std::vector<std::string> Keys;

  /**
   * The Reponse object received for the requests of etcd::Client
   */
  class Response
  {
  public:
    int64_t          revision = 0;
    etcd::Status     status;
    std::string      action;
    Value            value;
    Value            prev_value;
    Values           values;
    Keys             keys;

    template<typename T> static pplx::task<etcd::Response> create(
        std::shared_ptr<T> call,
        const pplx::task_options & task_options)
    {
      return pplx::task<etcd::Response>([call]()
      {
        call->waitForResponse();
        return etcd::Response(call->ParseResponse());
      }, task_options);
    }

    Response() = default;
    Response(etcd::StatusCode const etcd_error_code, std::string etcd_error_message);
    explicit Response(etcd::Status && status);
    explicit Response(etcd::Status const & status);
    explicit Response(etcdv3::V3Response && response);

    /**
     * Returns true if this is a successful response
     */
    bool is_ok() const;
    bool etcd_is_ok() const;
    bool grpc_is_ok() const;
  };
}

#endif
