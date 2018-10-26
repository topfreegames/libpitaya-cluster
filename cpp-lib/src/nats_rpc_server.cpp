#include "pitaya_nats.h"
#include "nats/nats.h"
#include "pitaya.h"

using std::string;
using namespace pitaya;

// TODO: configure rpc server
pitaya_nats::RPCServer::RPCServer(std::unique_ptr<Server> server, const pitaya_nats::NATSConfig &config)
{
    natsConnection_ConnectTo(&nc, config.nats_addr.c_str());
}