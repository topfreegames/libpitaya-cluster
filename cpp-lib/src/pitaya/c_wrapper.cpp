#include "pitaya/c_wrapper.h"

#include "pitaya/constants.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/grpc/rpc_client.h"
#include "pitaya/grpc/rpc_server.h"
#include "pitaya/nats/rpc_client.h"
#include "pitaya/nats/rpc_server.h"

#include "spdlog/logger.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <assert.h>
#include <boost/optional.hpp>
#include <chrono>
#include <cstdio>

using namespace std;
using namespace pitaya;
using namespace pitaya::service_discovery;
using namespace pitaya::etcdv3_service_discovery;
using pitaya::nats::NatsConfig;

static char*
ConvertToCString(const std::string& str)
{
    char* cString = (char*)calloc(str.size() + 1, 1);
    if (cString) {
        std::memcpy(cString, str.data(), str.size());
    }
    return cString;
}

static CsharpFreeCb freePinvoke;

static RpcPinvokeCb gPinvokeCb;
static std::shared_ptr<spdlog::logger> gLogger;
static void (*gSignalHandler)() = nullptr;

static CServer*
FromPitayaServer(const pitaya::Server& pServer)
{
    auto server = (CServer*)malloc(sizeof(CServer));
    server->id = ConvertToCString(pServer.id);
    server->type = ConvertToCString(pServer.type);
    server->metadata = ConvertToCString(pServer.metadata);
    server->hostname = ConvertToCString(pServer.hostname);
    server->frontend = pServer.frontend;
    return server;
}

static CPitayaError*
NewPitayaError(const std::string& codeStr, const std::string& msgStr)
{
    CPitayaError* err = (CPitayaError*)malloc(sizeof(CPitayaError));
    err->code = ConvertToCString(codeStr);
    err->msg = ConvertToCString(msgStr);
    return err;
}

static void
FreePitayaError(CPitayaError* err)
{
    free(err->code);
    free(err->msg);
    // free(err);
}

static void
FreeServer(CServer* sv)
{
    free(sv->id);
    free(sv->type);
    free(sv->metadata);
    free(sv->hostname);
    // free(sv);
}

pitaya::etcdv3_service_discovery::Config
CSDConfig::ToConfig()
{
    pitaya::etcdv3_service_discovery::Config config;
    config.endpoints = endpoints ? std::string(endpoints) : "";
    config.etcdPrefix = etcdPrefix ? std::string(etcdPrefix) : "";
    config.heartbeatTTLSec = std::chrono::seconds(heartbeatTTLSec);
    config.logHeartbeat = logHeartbeat;
    config.logServerSync = logServerSync;
    config.logServerDetails = logServerDetails;
    config.syncServersIntervalSec = std::chrono::seconds(syncServersIntervalSec);
    return config;
}

void
OnSignal(int signum)
{
    if (gSignalHandler) {
        gSignalHandler();
    }
}

void
print_data(void* dt, int sz)
{
    int j;
    printf("data on c: ");
    for (j = 0; j < sz; ++j)
        printf("%02x ", ((uint8_t*)dt)[j]);
    printf("\n");
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

    // for debugging purposes, uncomment the below line
    // print_data(memBuf->data, memBuf->size);
    bool success = res.ParseFromArray(memBuf->data, memBuf->size);
    if (!success) {
        auto err = new protos::Error();
        err->set_code(pitaya::constants::kCodeInternalError);
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    freePinvoke(memBuf->data);
    freePinvoke(memBuf);
    return res;
}

static std::shared_ptr<spdlog::logger>
CreateLogger(const char* logFile)
{
    std::shared_ptr<spdlog::logger> logger;
    if (logFile && strlen(logFile) > 0) {
        logger = spdlog::basic_logger_mt("c_wrapper", logFile);
    } else {
        logger = spdlog::stdout_color_mt("c_wrapper");
    }

    logger->set_level(spdlog::level::debug);
    return logger;
}

using ClusterPtr = void*;

extern "C"
{
    bool tfg_pitc_Initialize(CServer* sv,
                             CSDConfig* sdConfig,
                             CNATSConfig* nc,
                             RpcPinvokeCb cb,
                             CsharpFreeCb freeCb,
                             const char* logFile,
                             bool useGRPC)
    {
        if (!spdlog::get("c_wrapper")) {
            gLogger = CreateLogger(logFile);
        }

        switch (sdConfig->logLevel) {
            case LogLevel_Debug:
                gLogger->set_level(spdlog::level::debug);
                break;
            case LogLevel_Info:
                gLogger->set_level(spdlog::level::info);
                break;
            case LogLevel_Warn:
                gLogger->set_level(spdlog::level::warn);
                break;
            case LogLevel_Error:
                gLogger->set_level(spdlog::level::err);
                break;
            case LogLevel_Critical:
                gLogger->set_level(spdlog::level::critical);
                break;
        }

        NatsConfig natsCfg = NatsConfig(nc->addr ? std::string(nc->addr) : "",
                                        nc->requestTimeoutMs,
                                        nc->connectionTimeoutMs,
                                        nc->maxReconnectionAttempts,
                                        nc->maxPendingMsgs);

        Server server(sv->id ? std::string(sv->id) : "",
                      sv->type ? std::string(sv->type) : "",
                      sv->metadata ? std::string(sv->metadata) : "",
                      sv->hostname ? std::string(sv->hostname) : "",
                      sv->frontend);

        gPinvokeCb = cb;
        if (gPinvokeCb == nullptr) {
            gLogger->error("pinvoke callback not set!");
            return false;
        }

        freePinvoke = freeCb;
        if (freePinvoke == nullptr) {
            gLogger->error("freePinvoke callback not set!");
            return false;
        }

        try {
            if (useGRPC) {
                Cluster::Instance().InitializeWithGrpc(
                    sdConfig->ToConfig(), server, RpcCallback, "c_wrapper");
            } else {
                Cluster::Instance().InitializeWithNats(
                    sdConfig->ToConfig(), std::move(natsCfg), server, RpcCallback, "c_wrapper");
            }
            return true;
        } catch (const PitayaException& exc) {
            gLogger->error("Failed to create cluster instance: {}", exc.what());
            return false;
        }
    }

    bool tfg_pitc_GetServerById(const char* serverId, CServer* retServer)
    {
        auto maybeServer = Cluster::Instance().GetServiceDiscovery().GetServerById(serverId);

        if (!maybeServer) {
            return false;
        }

        pitaya::Server server = maybeServer.value();
        retServer->frontend = server.frontend; // FromPitayaServer(server);
        retServer->hostname = ConvertToCString(server.hostname);
        retServer->id = ConvertToCString(server.id);
        retServer->metadata = ConvertToCString(server.metadata);
        retServer->type = ConvertToCString(server.type);
        return true;
    }

    void tfg_pitc_FreeServer(CServer* cServer) { FreeServer(cServer); }

    void tfg_pitc_Terminate()
    {
        pitaya::Cluster::Instance().Terminate();
        gLogger->info("Cluster destroyed");
        gLogger->flush();
        spdlog::drop_all();
    }

    bool tfg_pitc_RPC(const char* serverId,
                      const char* route,
                      void* data,
                      int dataSize,
                      MemoryBuffer** outBuf,
                      CPitayaError* retErr)
    {
        assert(serverId && "server id should not be null");
        assert(route && "route should not be null");
        assert((data || (!data && dataSize == 0)) && "data len should be 0 if data is null");

        auto msg = new protos::Msg();
        msg->set_type(protos::MsgType::MsgRequest);
        msg->set_data(std::string(reinterpret_cast<char*>(data), dataSize));
        msg->set_route(route);

        protos::Request req;
        req.set_allocated_msg(msg);
        req.set_type(protos::RPCType::User);

        std::vector<uint8_t> buffer;
        buffer.resize(req.ByteSizeLong());

        req.SerializeToArray(buffer.data(), req.ByteSizeLong());

        protos::Response res;

        auto err = (!serverId || strlen(serverId) == 0)
                       ? Cluster::Instance().RPC(route, req, res)
                       : Cluster::Instance().RPC(serverId, route, req, res);

        if (err) {
            retErr->code = ConvertToCString(err->code);
            retErr->msg = ConvertToCString(err->msg);
            gLogger->error("received error on RPC: {}: {}", retErr->code, retErr->msg);
            return false;
        }

        size_t size = res.ByteSizeLong();
        uint8_t* bin = new uint8_t[size];

        if (!res.SerializeToArray(bin, size)) {
            gLogger->error("Error serializing RPC response");
            retErr->code = ConvertToCString(constants::kCodeInternalError);
            retErr->msg = ConvertToCString("Error serializing RPC response");
            return false;
        }

        *outBuf = new MemoryBuffer;
        (*outBuf)->size = size;
        (*outBuf)->data = bin;

        return true;
    }

    void tfg_pitc_FreeMem(void* mem) { free(mem); }

    void* tfg_pitc_AllocMem(int sz) { return malloc(sz); }

    void tfg_pitc_FreeMemoryBuffer(MemoryBuffer* buf)
    {
        delete[] reinterpret_cast<uint8_t*>(buf->data);
        delete buf;
    }

    void tfg_pitc_FreePitayaError(CPitayaError* err) { FreePitayaError(err); }

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
