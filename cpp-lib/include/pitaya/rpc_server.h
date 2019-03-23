#ifndef PITAYA_RPC_SERVER_H
#define PITAYA_RPC_SERVER_H

#include "pitaya.h"

namespace pitaya {

class RpcServer
{
public:
    RpcServer(pitaya::RpcHandlerFunc handler)
        : _handlerFunc(handler)
    {}

    virtual ~RpcServer() = default;
    virtual void ThreadStart() = 0;

protected:
    pitaya::RpcHandlerFunc _handlerFunc;
};

} // namespace pitaya

#endif // PITAYA_RPC_SERVER_H
