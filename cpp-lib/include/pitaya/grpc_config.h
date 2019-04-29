#ifndef PITAYA_GRPC_CONFIG_H
#define PITAYA_GRPC_CONFIG_H

#include <chrono>
#include <string>

namespace pitaya {

struct GrpcConfig
{
    std::string host;
    int32_t port;
    std::chrono::milliseconds serverShutdownDeadline;
    int32_t serverMaxNumberOfRpcs;

    GrpcConfig()
        : port(0)
        , serverShutdownDeadline(5)
        , serverMaxNumberOfRpcs(-1)
    {}
};

} // namespace pitaya

#endif // PITAYA_GRPC_CONFIG_H
