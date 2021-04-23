#include <etcd/Utils.hpp>


std::shared_ptr<grpc::Channel> etcd::utils::createChannel(
    const std::string & address,
    const std::shared_ptr<grpc::ChannelCredentials> & channel_credentials)
{
  const std::string substr("://");
  const auto i = address.find(substr);
  const std::string stripped_address = (i != std::string::npos) ? address.substr(i + substr.length()) : "";
  return grpc::CreateChannel(stripped_address, channel_credentials);
}
