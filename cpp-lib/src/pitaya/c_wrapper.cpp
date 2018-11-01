#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/nats/rpc_server.h"

using namespace std;
using namespace pitaya;
using pitaya::nats::NATSConfig;

struct CServer
{
    const char* id;
    const char* type;
    const char* metadata;
    const char* hostname;
    bool frontend;
};

struct CNATSConfig
{
    const char* addr;
    int64_t connection_timeout_ms;
    int request_timeout_ms;
    int max_reconnection_attempts;
    int max_pending_msgs;
};

struct RPCReq
{
    void* data;
    int data_len;
    const char* route;
};

struct RPCRes
{
    void* data;
    int data_len;
};

typedef RPCRes* (*rpc_pinvoke_cb)(RPCReq*);

rpc_pinvoke_cb pinvoke_cb;

protos::Response
rpc_cb(protos::Request req)
{
    protos::Response res;
    return res;
}

extern "C"
{

    // void tfg_ra_SetLogFunction(void (*LogFn)(int level, const char *msg))
    //{
    //}

    bool tfg_pitc_Initialize(CServer* sv, CNATSConfig* nc, rpc_pinvoke_cb cb)
    {
        NATSConfig nats_cfg = NATSConfig(nc->addr,
                                         nc->request_timeout_ms,
                                         nc->connection_timeout_ms,
                                         nc->max_reconnection_attempts,
                                         nc->max_pending_msgs);

        Server server =
            Server(sv->id, sv->type, sv->metadata, sv->hostname, sv->frontend);

        pinvoke_cb = cb;
        if (pinvoke_cb == NULL) {
            // TODO: logging
            return false;
        }
        // get configs
        return pitaya::Cluster::Instance().Init(nats_cfg, server, rpc_cb);
    }

    void tfg_pitc_Terminate() {}
}