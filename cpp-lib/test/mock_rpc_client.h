#ifndef MOCK_RPC_CLIENT_H
#define MOCK_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/rpc_client.h"

#include <gmock/gmock.h>

class MockRpcClient : public pitaya::RpcClient
{
public:
    MOCK_METHOD2(Call, protos::Response(const pitaya::Server&, const protos::Request&));
    MOCK_METHOD4(SendPushToUser, protos::Response(const std::string& user_id, const std::string& server_id, const std::string& server_type, const protos::Push& push));
};

#endif // MOCK_RPC_CLIENT_H
