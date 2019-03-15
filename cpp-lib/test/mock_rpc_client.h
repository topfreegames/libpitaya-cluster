#ifndef MOCK_RPC_CLIENT_H
#define MOCK_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/rpc_client.h"

#include <gmock/gmock.h>

class MockRpcClient : public pitaya::RpcClient
{
public:
    MOCK_METHOD2(Call, protos::Response(const pitaya::Server&, const protos::Request&));
};

#endif // MOCK_RPC_CLIENT_H
