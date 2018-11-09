#ifndef MOCK_RPC_CLIENT_H
#define MOCK_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/rpc_client.h"
#include "trompeloeil.hpp"

class MockRpcClient : public pitaya::RpcClient
{
public:
    MAKE_MOCK2(Call, protos::Response(const pitaya::Server&, const protos::Request&), override);
};

#endif // MOCK_RPC_CLIENT_H
