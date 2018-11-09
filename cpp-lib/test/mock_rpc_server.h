#ifndef MOCK_RPC_SERVER_H
#define MOCK_RPC_SERVER_H

#include "pitaya/rpc_server.h"

class MockRpcServer : public pitaya::RpcServer
{
public:
    MockRpcServer(pitaya::RpcHandlerFunc handler)
        : pitaya::RpcServer(handler)
    {}
};

#endif // MOCK_RPC_SERVER_H
