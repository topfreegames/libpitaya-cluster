#ifndef MOCK_RPC_SERVER_H
#define MOCK_RPC_SERVER_H

#include "pitaya/rpc_server.h"

#include <gmock/gmock.h>

class MockRpcServer : public pitaya::RpcServer
{
public:
    MOCK_METHOD1(Start, void(pitaya::RpcHandlerFunc));
    MOCK_METHOD0(Shutdown, void());
};

#endif // MOCK_RPC_SERVER_H
