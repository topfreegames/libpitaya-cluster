#ifndef MOCK_RPC_CLIENT_H
#define MOCK_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/rpc_client.h"

#include <gmock/gmock.h>

class MockRpcClient : public pitaya::RpcClient
{
public:
    MOCK_METHOD2(Call, protos::Response(const pitaya::Server&, const protos::Request&));
    MOCK_METHOD3(SendPushToUser,
                 protos::Response(const std::string& server_id,
                                  const std::string& server_type,
                                  const protos::Push& push));
    MOCK_METHOD3(SendKickToUser,
                 protos::KickAnswer(const std::string& server_id,
                                    const std::string& server_type,
                                    const protos::KickMsg& kick));
};

#endif // MOCK_RPC_CLIENT_H
