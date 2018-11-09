#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/constants.h"
#include "pitaya/nats/rpc_server.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <assert.h>
#include <boost/optional.hpp>
#include <chrono>

using namespace std;
using namespace pitaya;
using pitaya::nats::NatsConfig;

template<typename Mutex>
class function_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    function_sink(std::function<void(const char*)> handler)
        : _handler(handler)
    {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        fmt::memory_buffer formatted;
        spdlog::sinks::sink::formatter_->format(msg, formatted);
        std::string formattedStr = fmt::to_string(formatted);
        _handler(formattedStr.c_str());
    }

    void flush_() override
    { /* Ignore flushing */
    }

private:
    std::function<void(const char*)> _handler;
};

using function_sink_mt = function_sink<std::mutex>;
using function_sink_st = function_sink<spdlog::details::null_mutex>;

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

typedef MemoryBuffer* (*RpcPinvokeCb)(RPCReq*);
static RpcPinvokeCb gPinvokeCb;
static std::shared_ptr<spdlog::logger> gLogger;
static void (*gSignalHandler)() = nullptr;

struct CSDConfig
{
    const char* endpoints;
    const char* etcdPrefix;
    int heartbeatTTLSec;
    int logHeartbeat;
    int logServerSync;
    int logServerDetails;
    int syncServersIntervalSec;

    etcdv3_service_discovery::Config ToConfig()
    {
        etcdv3_service_discovery::Config config;
        config.endpoints = std::string(endpoints);
        config.etcdPrefix = std::string(etcdPrefix);
        config.heartbeatTTLSec = std::chrono::seconds(heartbeatTTLSec);
        config.logHeartbeat = logHeartbeat;
        config.logServerSync = logServerSync;
        config.logServerDetails = logServerDetails;
        config.syncServersIntervalSec = std::chrono::seconds(syncServersIntervalSec);
        return config;
    }
};

struct CNATSConfig
{
    const char* addr;
    int64_t connectionTimeoutMs;
    int requestTimeoutMs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;
};

void
OnSignal(int signum)
{
    if (gSignalHandler) {
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
        err->set_code(pitaya::kCodeInternalError);
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    free(memBuf->data);
    free(memBuf);
    return res;
}

static std::shared_ptr<spdlog::logger>
CreateLogger(void (*logHandler)(const char*))
{
    std::shared_ptr<spdlog::logger> logger;
    if (logHandler) {
        logger = std::make_shared<spdlog::logger>("c_wrapper",
                                                  std::make_shared<function_sink_mt>(logHandler));
        spdlog::register_logger(logger);
    } else {
        logger = spdlog::stdout_color_mt("c_wrapper");
    }

    logger->set_level(spdlog::level::debug);
    return logger;
}

using ClusterPtr = void*;

extern "C"
{

    ClusterPtr tfg_pitc_NewCluster(CServer* sv,
                                   CSDConfig* sdConfig,
                                   CNATSConfig* nc,
                                   RpcPinvokeCb cb,
                                   void (*logHandler)(const char*))
    {
        gLogger = CreateLogger(logHandler);

        NatsConfig natsCfg = NatsConfig(nc->addr,
                                        nc->requestTimeoutMs,
                                        nc->connectionTimeoutMs,
                                        nc->maxReconnectionAttempts,
                                        nc->maxPendingMsgs);

        Server server(sv->id, sv->type, sv->metadata, sv->hostname, sv->frontend);

        gPinvokeCb = cb;
        if (gPinvokeCb == nullptr) {
            gLogger->error("pinvoke callback not set!");
            return nullptr;
        }

        try {
            auto cluster = new pitaya::Cluster(
                sdConfig->ToConfig(), std::move(natsCfg), server, RpcCallback, "c_wrapper");
            return reinterpret_cast<ClusterPtr>(cluster);
        } catch (const PitayaException& exc) {
            gLogger->error("Failed to create cluster instance: {}", exc.what());
            return nullptr;
        }
    }

    CServer* tfg_pitc_GetServerById(ClusterPtr ptr, const char* serverId)
    {
        auto cluster = reinterpret_cast<pitaya::Cluster*>(ptr);
        auto maybeServer = cluster->GetServiceDiscovery().GetServerById(serverId);

        if (!maybeServer) {
            return nullptr;
        }

        pitaya::Server server = maybeServer.value();
        auto cs = CServer::FromPitayaServer(server);
        return cs;
    }

    void tfg_pitc_FreeServer(CServer* cServer) { delete cServer; }

    void tfg_pitc_DestroyCluster(ClusterPtr ptr)
    {
        delete reinterpret_cast<pitaya::Cluster*>(ptr);
        gLogger->info("Cluster destroyed");
    }

    CPitayaError* tfg_pitc_RPC(ClusterPtr ptr,
                               const char* serverId,
                               const char* route,
                               void* data,
                               int dataSize,
                               MemoryBuffer** outBuf)
    {
        assert(serverId && "server id should not be null");
        assert(route && "route should not be null");
        assert((data || (!data && dataSize == 0)) && "data len should be 0 if data is null");

        auto cluster = reinterpret_cast<pitaya::Cluster*>(ptr);

        auto msg = new protos::Msg();
        msg->set_data(std::string(reinterpret_cast<char*>(data), dataSize));
        msg->set_route(route);

        protos::Request req;
        req.set_allocated_msg(msg);

        std::vector<uint8_t> buffer;
        buffer.resize(req.ByteSizeLong());

        req.SerializeToArray(buffer.data(), req.ByteSizeLong());

        protos::Response res;

        auto err = (strlen(serverId) == 0) ? cluster->RPC(route, req, res)
                                           : cluster->RPC(serverId, route, req, res);

        if (err) {
            gLogger->error("received error on RPC: {}", err->msg);
            return new CPitayaError(err->code, err->msg);
        }

        size_t size = res.ByteSizeLong();
        uint8_t* bin = new uint8_t[size];

        if (!res.SerializeToArray(bin, size)) {
            gLogger->error("Error serializing RPC response");
            // TODO: what should be done here?
            return nullptr;
        }

        *outBuf = new MemoryBuffer;
        (*outBuf)->size = size;
        (*outBuf)->data = bin;

        return nullptr;
    }

    void tfg_pitc_FreeMemoryBuffer(MemoryBuffer* buf)
    {
        delete[] reinterpret_cast<uint8_t*>(buf->data);
        delete buf;
    }

    void tfg_pitc_FreePitayaError(PitayaError* err) { delete err; }

    void tfg_pitc_OnSignal(void (*signalHandler)())
    {
        if (gLogger) {
            gLogger->info("Adding signal handler");
        }
        gSignalHandler = signalHandler;
        signal(SIGINT, OnSignal);
        signal(SIGTERM, OnSignal);
        signal(SIGKILL, OnSignal);
    }
}
