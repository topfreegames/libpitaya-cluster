#ifndef PITAYA_RPC_CLIENT_H
#define PITAYA_RPC_CLIENT_H

#include "pitaya.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"

namespace pitaya {

class RpcClient
{
public:
    virtual ~RpcClient() = default;
    virtual protos::Response Call(const pitaya::Server& target, const protos::Request& req) = 0;
};

} // namespace pitaya

#endif // PITAYA_RPC_CLIENT_H
