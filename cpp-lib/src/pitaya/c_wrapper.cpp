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
    server->id = ConvertToCString(pServer.Id());
    server->type = ConvertToCString(pServer.Type());
    server->metadata = ConvertToCString(pServer.Metadata());
    server->hostname = ConvertToCString(pServer.Hostname());
    server->frontend = pServer.IsFrontend();
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

pitaya::GrpcConfig
CGrpcConfig::ToConfig()
{
    pitaya::GrpcConfig config;
    config.host = host;
    config.port = port;
    config.connectionTimeout = std::chrono::seconds(connectionTimeoutSec);
    return config;
}

pitaya::EtcdBindingStorageConfig
CBindingStorageConfig::ToConfig()
{
    pitaya::EtcdBindingStorageConfig config;
    config.endpoint = endpoint;
    config.etcdPrefix = etcdPrefix;
    config.leaseTtl = std::chrono::seconds(leaseTtlSec);
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

void
RpcCallback(protos::Request req, pitaya::Rpc* rpc)
{
    protos::Response res;
    MemoryBuffer reqBuffer;
    size_t size = req.ByteSizeLong();
    reqBuffer.data = malloc(size);
    reqBuffer.size = size;

    bool success = req.SerializeToArray(reqBuffer.data, size);
    if (!success) {
        gLogger->error("failed to serialize protobuf request!");
        // TODO what to do here ? error like below
    }

    auto memBuf = gPinvokeCb(&reqBuffer);

    // for debugging purposes, uncomment the below line
    // print_data(memBuf->data, memBuf->size);
    success = res.ParseFromArray(memBuf->data, memBuf->size);
    if (!success) {
        auto err = new protos::Error();
        err->set_code(pitaya::constants::kCodeInternalError);
        err->set_msg("pinvoke failed");
        res.set_allocated_error(err);
    }
    // TODO hacky, OMG :O - do stress testing to see if this will leak memory
    freePinvoke(memBuf->data);
    freePinvoke(memBuf);
    free(reqBuffer.data);

    rpc->Finish(res);
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

static pitaya::Server
CServerToServer(CServer* sv)
{
    auto id = sv->id ? std::string(sv->id) : "";
    auto type = sv->type ? std::string(sv->type) : "";
    auto hostname = sv->hostname ? std::string(sv->hostname) : "";
    auto metadata = sv->metadata ? std::string(sv->metadata) : "";

    auto s =
        pitaya::Server((Server::Kind)sv->frontend, id, type, hostname).WithRawMetadata(metadata);
    return s;
}

static void
SetLogLevel(LogLevel level)
{
    switch (level) {
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
}

extern "C"
{
    bool tfg_pitc_InitializeWithGrpc(CGrpcConfig* grpcConfig,
                                     CSDConfig* sdConfig,
                                     CServer* sv,
                                     LogLevel logLevel,
                                     const char* logFile)
    {
        if (!spdlog::get("c_wrapper")) {
            gLogger = CreateLogger(logFile);
        }
        SetLogLevel(logLevel);
        Server server = CServerToServer(sv);
        // TODO: make a setter function
        CBindingStorageConfig bindingStorageConfig;
        bindingStorageConfig.endpoint = sdConfig->endpoints;
        bindingStorageConfig.etcdPrefix = "pitaya/";
        bindingStorageConfig.leaseTtlSec = 5;

        try {
            Cluster::Instance().InitializeWithGrpc(grpcConfig->ToConfig(),
                                                   sdConfig->ToConfig(),
                                                   bindingStorageConfig.ToConfig(),
                                                   server,
                                                   "c_wrapper");

            return true;
        } catch (const PitayaException& exc) {
            gLogger->error("Failed to create cluster instance: {}", exc.what());
            return false;
        }
    }

    bool tfg_pitc_InitializeWithNats(CNATSConfig* nc,
                                     CSDConfig* sdConfig,
                                     CServer* sv,
                                     LogLevel logLevel,
                                     const char* logFile)
    {
        if (!spdlog::get("c_wrapper")) {
            gLogger = CreateLogger(logFile);
        }
        SetLogLevel(logLevel);
        NatsConfig natsCfg = NatsConfig(nc->addr ? std::string(nc->addr) : "",
                                        nc->requestTimeoutMs,
                                        nc->connectionTimeoutMs,
                                        nc->maxReconnectionAttempts,
                                        nc->maxPendingMsgs);
        Server server = CServerToServer(sv);

        try {
            Cluster::Instance().InitializeWithNats(
                std::move(natsCfg), sdConfig->ToConfig(), server, "c_wrapper");
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
        retServer->frontend = server.IsFrontend(); // FromPitayaServer(server);
        retServer->hostname = ConvertToCString(server.Hostname());
        retServer->id = ConvertToCString(server.Id());
        retServer->metadata = ConvertToCString(server.Metadata());
        retServer->type = ConvertToCString(server.Type());
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

    static bool sendResponseToManaged(MemoryBuffer** outBuf,
                                      const protos::Response& res,
                                      CPitayaError*& retErr)
    {
        size_t size = res.ByteSizeLong();
        uint8_t* bin = new uint8_t[size];

        if (!res.SerializeToArray(bin, size)) {
            retErr->code = ConvertToCString(constants::kCodeInternalError);
            retErr->msg = ConvertToCString("Error serializing response");
            return false;
        }

        *outBuf = new MemoryBuffer;
        (*outBuf)->size = size;
        (*outBuf)->data = bin;

        return true;
    }

    bool tfg_pitc_SendPushToUser(const char* server_id,
                                 const char* server_type,
                                 MemoryBuffer* memBuf,
                                 MemoryBuffer** outBuf,
                                 CPitayaError* retErr)
    {
        protos::Push push;
        protos::Response res;

        bool success = push.ParseFromArray(memBuf->data, memBuf->size);
        if (!success) {
            retErr->code = ConvertToCString(pitaya::constants::kCodeInternalError);
            retErr->msg = ConvertToCString("failed to deserialize push");
            return false;
        }

        auto err = Cluster::Instance().SendPushToUser(server_id, server_type, push, res);

        if (err) {
            retErr->code = ConvertToCString(err->code);
            retErr->msg = ConvertToCString(err->msg);
            return false;
        }

        return sendResponseToManaged(outBuf, res, retErr);
    }

    bool tfg_pitc_SendKickToUser(const char* server_id,
                                 const char* server_type,
                                 MemoryBuffer* memBuf,
                                 MemoryBuffer** outBuf,
                                 CPitayaError* retErr)
    {
        protos::KickMsg kick;
        protos::KickAnswer res;

        bool success = kick.ParseFromArray(memBuf->data, memBuf->size);
        if (!success) {
            retErr->code = ConvertToCString(pitaya::constants::kCodeInternalError);
            retErr->msg = ConvertToCString("failed to deserialize kick msg");
            return false;
        }

        auto err = Cluster::Instance().SendKickToUser(server_id, server_type, kick, res);

        if (err) {
            retErr->code = ConvertToCString(err->code);
            retErr->msg = ConvertToCString(err->msg);
            return false;
        }

        size_t size = res.ByteSizeLong();
        uint8_t* bin = new uint8_t[size];

        if (!res.SerializeToArray(bin, size)) {
            retErr->code = ConvertToCString(pitaya::constants::kCodeInternalError);
            retErr->msg = ConvertToCString("failed to serialize kick ans");
            return false;
        }

        *outBuf = new MemoryBuffer;
        (*outBuf)->size = size;
        (*outBuf)->data = bin;

        return true;
    }

    bool tfg_pitc_RPC(const char* serverId,
                      const char* route,
                      void* data,
                      int dataSize,
                      MemoryBuffer** outBuf,
                      CPitayaError* retErr)
    {
        if (serverId == NULL || route == NULL || retErr == NULL) {
            gLogger->error("Received null arguments on tfg_pic_RPC");
            retErr->code = ConvertToCString(constants::kCodeInternalError);
            retErr->msg = ConvertToCString("Received NULL arguments on tfg_pitc_RPC");
            return false;
        }

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

        return sendResponseToManaged(outBuf, res, retErr);
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

    struct CRpc
    {
        MemoryBuffer* req;
        void* tag;
    };

    void tfg_pitc_FinishRpcCall(MemoryBuffer* mb, void* tagPtr)
    {
        auto rpc = reinterpret_cast<pitaya::Rpc*>(tagPtr);

        protos::Response res;

        bool success = res.ParseFromArray(mb->data, mb->size);
        if (!success) {
            auto err = new protos::Error();
            err->set_code(pitaya::constants::kCodeInternalError);
            err->set_msg("pinvoke failed");
            res.set_allocated_error(err);
        }

        rpc->Finish(res);
        // TODO: Hacky, alloced on c#, may leak
        free(mb->data);
        free(mb);
    }

    CRpc* tfg_pitc_WaitForRpc()
    {
        Cluster::RpcData rpcData = Cluster::Instance().WaitForRpc();

        MemoryBuffer* reqBuffer = new MemoryBuffer();
        size_t size = rpcData.req.ByteSizeLong();
        reqBuffer->data = malloc(size);
        reqBuffer->size = size;

        bool success = rpcData.req.SerializeToArray(reqBuffer->data, size);
        if (!success) {
            // TODO: send in response?
            gLogger->error("failed to serialize protobuf request!");
        }

        CRpc* crpc = new CRpc();
        crpc->req = reqBuffer;
        crpc->tag = rpcData.rpc;

        return crpc;
    }
}
