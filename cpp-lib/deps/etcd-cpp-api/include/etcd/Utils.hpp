#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <grpc++/grpc++.h>

namespace etcd {

  namespace utils {

    std::shared_ptr<grpc::Channel> createChannel(
        const std::string & address,
        const std::shared_ptr<grpc::ChannelCredentials> & channel_credentials = grpc::InsecureChannelCredentials());

  } // namespace utils

} // namespace etcd

#endif // UTILS_H
