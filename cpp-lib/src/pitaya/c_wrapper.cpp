#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/nats/rpc_server.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <assert.h>
#include <boost/optional.hpp>

using namespace std;
using namespace pitaya;
using pitaya::nats::NATSConfig;

struct CServer
{
    const char* id = nullptr;
    const char* type = nullptr;
    const char* metadata = nullptr;
    const char* hostname = nullptr;
    bool frontend = false;
};

struct CPitayaError
{
    char* code = nullptr;
    char* msg = nullptr;

    CPitayaError(const std::string& codeStr, const std::string& msgStr)
    {
        code = new char[codeStr.size() + 1]();
        std::memcpy(code, codeStr.data(), codeStr.size());

        msg = new char[msgStr.size() + 1]();
        std::memcpy(msg, msgStr.data(), msgStr.size());
    }

    ~CPitayaError()
    {
        delete[] code;
        delete[] msg;
    }
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

struct RpcResult
{
    char* data;
    CPitayaError* error;

    static RpcResult* Error(const std::string& code, const std::string& msg)
    {
        auto res = new RpcResult;
        res->data = nullptr;
        res->error = new CPitayaError(code, msg);
        return res;
    }

    static RpcResult* Data(const std::string& data)
    {
        auto res = new RpcResult;
        res->data = nullptr;
        res->error = nullptr;

        res->data = new char[data.size() + 1]();
        std::memcpy(res->data, data.data(), data.size());

        return res;
    }

    ~RpcResult()
    {
        delete[] data;
        delete error;
    }
};

typedef RPCRes* (*RpcPinvokeCb)(RPCReq*);
static RpcPinvokeCb gPinvokeCb;
static std::shared_ptr<spdlog::logger> gLogger = spdlog::stdout_color_mt("c_wrapper");

protos::Response
RpcCallback(protos::Request req)
{
    protos::Response res;

    RPCReq cReq;
    cReq.data = (void*)req.msg().data().data();
    cReq.data_len = req.msg().data().size();
    cReq.route = req.msg().route().c_str();
    auto pinvokeRes = gPinvokeCb(&cReq);

    bool success = res.ParseFromArray(pinvokeRes->data, pinvokeRes->data_len);
    if (!success) {
        auto err = new protos::Error();
        err->set_code("PIT-500");
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    free(pinvokeRes->data);
    free(pinvokeRes);
    return res;
}

static const char*
ConvertToCString(const std::string& str)
{
    char* cString = reinterpret_cast<char*>(std::calloc(str.size(), 1));
    std::memcpy(cString, str.data(), str.size());
    return cString;
}

extern "C"
{

    // void tfg_ra_SetLogFunction(void (*LogFn)(int level, const char *msg))
    //{
    //}

    bool tfg_pitc_Initialize(CServer* sv, CNATSConfig* nc, RpcPinvokeCb cb)
    {
        NATSConfig natsCfg = NATSConfig(nc->addr,
                                        nc->request_timeout_ms,
                                        nc->connection_timeout_ms,
                                        nc->max_reconnection_attempts,
                                        nc->max_pending_msgs);

        Server server(sv->id, sv->type, sv->metadata, sv->hostname, sv->frontend);

        gPinvokeCb = cb;
        if (gPinvokeCb == nullptr) {
            gLogger->error("pinvoke callback not set!");
            return false;
        }
        // get configs
        return pitaya::Cluster::Instance().Initialize(std::move(natsCfg), server, RpcCallback);
    }

    CServer* tfg_pitc_GetServerById(const char* serverId)
    {
        auto maybeServer =
            pitaya::Cluster::Instance().GetServiceDiscovery().GetServerById(serverId);

        if (maybeServer) {
            pitaya::Server server = maybeServer.value();
            CServer* cs = reinterpret_cast<CServer*>(malloc(sizeof(CServer)));
            cs->frontend = server.frontend;
            cs->hostname = ConvertToCString(server.hostname);
            cs->id = ConvertToCString(server.id);
            cs->type = ConvertToCString(server.type);
            cs->metadata = ConvertToCString(server.metadata);
            return cs;
        }

        return nullptr;
    }

    void tfg_pitc_FreeServer(CServer* cServer)
    {
        std::free((void*)cServer->id);
        std::free((void*)cServer->type);
        std::free((void*)cServer->metadata);
        std::free((void*)cServer->hostname);
        std::free((void*)cServer);
    }

    void tfg_pitc_Shutdown() { pitaya::Cluster::Instance().Shutdown(); }

    RpcResult* tfg_pitc_RPC(const char* serverId, const char* route, const char* data)
    {
        assert(serverId);
        assert(route);
        assert(data);

        auto msg = new protos::Msg();
        msg->set_data(data);
        msg->set_route(route);

        auto req = std::make_shared<protos::Request>();
        req->set_allocated_msg(msg);

        std::vector<uint8_t> buffer;
        buffer.resize(req->ByteSizeLong());

        req->SerializeToArray(buffer.data(), req->ByteSizeLong());

        auto res = std::make_shared<protos::Response>();
        auto err = pitaya::Cluster::Instance().RPC(serverId, route, req, res);

        if (err) {
            gLogger->error("received error on RPC: {} ", err->msg);
            return RpcResult::Error(err->code, err->msg);
        }

        return RpcResult::Data(res->data());
    }

    void tfg_pitc_FreeRpcResult(RpcResult* res) { delete res; }
}
