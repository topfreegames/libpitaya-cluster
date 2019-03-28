#ifndef PITAYA_RPC_CLIENT_H
#define PITAYA_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/protos/kick.pb.h"
#include "pitaya/protos/push.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"

namespace pitaya {

class RpcClient
{
public:
    virtual ~RpcClient() = default;
    virtual protos::Response Call(const pitaya::Server& target, const protos::Request& req) = 0;
    virtual protos::Response SendPushToUser(const std::string& server_id,
                                            const std::string& server_type,
                                            const protos::Push& push) = 0;
    virtual protos::KickAnswer SendKickToUser(const std::string& server_id,
                                              const std::string& server_type,
                                              const protos::KickMsg& kick) = 0;
};

} // namespace pitaya

#endif // PITAYA_RPC_CLIENT_H
