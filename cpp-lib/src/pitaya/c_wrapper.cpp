#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/nats/rpc_server.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

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
static rpc_pinvoke_cb pinvoke_cb;
static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("c_wrapper");

protos::Response
rpc_cb(protos::Request req)
{
    protos::Response res;

    RPCReq c_req;
    c_req.data = (void*)req.msg().data().data();
    c_req.data_len = req.msg().data().size();
    c_req.route = req.msg().route().c_str();
    auto pinvoke_res = pinvoke_cb(&c_req);

    bool success = res.ParseFromArray(pinvoke_res->data, pinvoke_res->data_len);
    if (!success) {
        auto err = new protos::Error();
        err->set_code("PIT-500");
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    free(pinvoke_res->data);
    free(pinvoke_res);
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

        Server server = Server(sv->id, sv->type, sv->metadata, sv->hostname, sv->frontend);

        pinvoke_cb = cb;
        if (pinvoke_cb == NULL) {
            logger->error("pinvoke callback not set!");
            return false;
        }
        // get configs
        return pitaya::Cluster::Instance().Initialize(std::move(nats_cfg), server, rpc_cb);
    }

    void tfg_pitc_Terminate() {}
}