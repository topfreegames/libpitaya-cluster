#ifndef PITAYA_RPC_SERVER_H
#define PITAYA_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/protos/response.grpc.pb.h"

namespace pitaya {

class RpcServer
{
public:
    virtual ~RpcServer() = default;

    virtual void Start(pitaya::RpcHandlerFunc handler) = 0;

    virtual void Shutdown() = 0;
};

} // namespace pitaya

#endif // PITAYA_RPC_SERVER_H
