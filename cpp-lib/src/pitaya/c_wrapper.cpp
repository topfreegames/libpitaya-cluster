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

static char*
ConvertToCString(const std::string& str)
{
    char* cString = new char[str.size() + 1]();
    std::memcpy(cString, str.data(), str.size());
    return cString;
}

struct CServer
{
    char* id = nullptr;
    char* type = nullptr;
    char* metadata = nullptr;
    char* hostname = nullptr;
    bool frontend = false;

    ~CServer()
    {
        delete[] id;
        delete[] type;
        delete[] metadata;
        delete[] hostname;
    }

    static CServer* FromPitayaServer(const pitaya::Server& pServer)
    {
        auto server = new CServer;
        server->id = ConvertToCString(pServer.id);
        server->type = ConvertToCString(pServer.type);
        server->metadata = ConvertToCString(pServer.metadata);
        server->hostname = ConvertToCString(pServer.hostname);
        server->frontend = pServer.frontend;
        return server;
    }
};

struct CPitayaError
{
    char* code = nullptr;
    char* msg = nullptr;

    CPitayaError(const std::string& codeStr, const std::string& msgStr)
    {
        code = ConvertToCString(codeStr);
        msg = ConvertToCString(msgStr);
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

struct MemoryBuffer
{
    void* data;
    int size;
};

struct RPCReq
{
    MemoryBuffer buffer;
    const char* route;
};

struct RPCCallResult
{
    char* data;
    CPitayaError* error;

    static RPCCallResult* Error(const std::string& code, const std::string& msg)
    {
        auto res = new RPCCallResult;
        res->data = nullptr;
        res->error = new CPitayaError(code, msg);
        return res;
    }

    static RPCCallResult* Data(const std::string& data)
    {
        auto res = new RPCCallResult;
        res->data = nullptr;
        res->error = nullptr;

        res->data = new char[data.size() + 1]();
        std::memcpy(res->data, data.data(), data.size());

        return res;
    }

    ~RPCCallResult()
    {
        delete[] data;
        delete error;
    }
};

typedef MemoryBuffer* (*RpcPinvokeCb)(RPCReq*);
static RpcPinvokeCb gPinvokeCb;
static std::shared_ptr<spdlog::logger> gLogger = spdlog::stdout_color_mt("c_wrapper");
static void (*gSignalHandler)() = nullptr;

void
OnSignal(int signum)
{
    gLogger->error("OIQWJDOIQJWDOIJQWOIDJQOWIJDOIQWJDOIQJWDOIJQWODIJQWOIDJOQWIJOIQWJDOIJQWDOIJQWOID"
                   "JQOWIJDOQWIJDOQWIJD");
    if (gSignalHandler) {
        gLogger->info("Calling c# signal handler");
        gSignalHandler();
    }
}

protos::Response
RpcCallback(protos::Request req)
{
    protos::Response res;

    RPCReq cReq;
    cReq.buffer.data = (void*)req.msg().data().data();
    cReq.buffer.size = req.msg().data().size();
    cReq.route = req.msg().route().c_str();
    auto memBuf = gPinvokeCb(&cReq);

    bool success = res.ParseFromArray(memBuf->data, memBuf->size);
    if (!success) {
        auto err = new protos::Error();
        err->set_code("PIT-500");
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    free(memBuf->data);
    free(memBuf);
    return res;
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

        pitaya::cluster::LogOptions logOpts;
        logOpts.serviceDiscovery.logLeaseKeepAlive = false;
        logOpts.serviceDiscovery.logServerDetails = false;

        // get configs
        return pitaya::Cluster::Instance().Initialize(
            std::move(natsCfg), logOpts, server, RpcCallback);
    }

    CServer* tfg_pitc_GetServerById(const char* serverId)
    {
        auto maybeServer =
            pitaya::Cluster::Instance().GetServiceDiscovery().GetServerById(serverId);

        if (!maybeServer) {
            return nullptr;
        }

        pitaya::Server server = maybeServer.value();
        auto cs = CServer::FromPitayaServer(server);
        return cs;
    }

    void tfg_pitc_FreeServer(CServer* cServer) { delete cServer; }

    void tfg_pitc_Shutdown() { pitaya::Cluster::Instance().Shutdown(); }

    CPitayaError* tfg_pitc_RPC(const char* serverId,
                               const char* route,
                               void* data,
                               int dataSize,
                               MemoryBuffer* outBuf)
    {
        assert(serverId);
        assert(route);
        assert(data);

        auto msg = new protos::Msg();
        msg->set_data(std::string(reinterpret_cast<char*>(data), dataSize));
        msg->set_route(route);

        auto req = std::make_shared<protos::Request>();
        req->set_allocated_msg(msg);

        std::vector<uint8_t> buffer;
        buffer.resize(req->ByteSizeLong());

        req->SerializeToArray(buffer.data(), req->ByteSizeLong());

        auto res = std::make_shared<protos::Response>();
        auto err = pitaya::Cluster::Instance().RPC(serverId, route, req, res);

        if (err) {
            gLogger->error("received error on RPC: {}", err->msg);
            return new CPitayaError(err->code, err->msg);
        }

        uint8_t* bin = new uint8_t[res->data().size()];
        std::memcpy(bin, res->data().data(), res->data().size());

        outBuf->size = res->data().size();
        outBuf->data = bin;
        return nullptr;
    }

    void tfg_pitc_FreeMemoryBuffer(MemoryBuffer* mb)
    {
        delete[] reinterpret_cast<uint8_t*>(mb->data);
    }

    void tfg_pitc_FreePitayaError(PitayaError* err)
    {
        delete err;
    }

    void tfg_pitc_OnSignal(void (*signalHandler)())
    {
        gLogger->info("Adding signal handler");
        gSignalHandler = signalHandler;
        signal(SIGINT, OnSignal);
        signal(SIGTERM, OnSignal);
        signal(SIGKILL, OnSignal);
    }
}
