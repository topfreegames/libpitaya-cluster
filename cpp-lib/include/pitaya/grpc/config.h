#ifndef PITAYA_GRPC_CONFIG_H
#define PITAYA_GRPC_CONFIG_H

#include <chrono>
#include <string>

namespace pitaya {

struct GrpcConfig
{
    std::string host;
    int port;
    std::chrono::milliseconds connectionTimeout;

    GrpcConfig()
        : port(0)
        , connectionTimeout(4)
    {}
};

} // namespace pitaya

#endif // PITAYA_GRPC_CONFIG_H
