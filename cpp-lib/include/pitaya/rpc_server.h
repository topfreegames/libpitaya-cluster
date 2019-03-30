#ifndef PITAYA_RPC_SERVER_H
#define PITAYA_RPC_SERVER_H

#include "pitaya.h"
#include "pitaya/protos/response.grpc.pb.h"

namespace pitaya {

class RpcServer
{
public:
    RpcServer(pitaya::RpcHandlerFunc handler)
        : _handlerFunc(handler)
    {}

    virtual ~RpcServer() = default;

protected:
    pitaya::RpcHandlerFunc _handlerFunc;
};

} // namespace pitaya

#endif // PITAYA_RPC_SERVER_H
