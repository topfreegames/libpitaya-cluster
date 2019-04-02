#ifndef PITAYA_RPC_CLIENT_H
#define PITAYA_RPC_CLIENT_H

#include "pitaya.h"
#include "pitaya/protos/kick.pb.h"
#include "pitaya/protos/push.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"

#include <boost/optional.hpp>

namespace pitaya {

class RpcClient
{
public:
    virtual ~RpcClient() = default;
    virtual protos::Response Call(const pitaya::Server& target, const protos::Request& req) = 0;
    virtual boost::optional<PitayaError> SendPushToUser(const std::string& serverId,
                                                        const std::string& serverType,
                                                        const protos::Push& push) = 0;
    virtual boost::optional<PitayaError> SendKickToUser(const std::string& serverId,
                                                        const std::string& serverType,
                                                        const protos::KickMsg& kick) = 0;
};

} // namespace pitaya

#endif // PITAYA_RPC_CLIENT_H
