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
#include <cpprest/json.h>

using namespace std;
using namespace pitaya;
using namespace pitaya::service_discovery;
using namespace pitaya::etcdv3_service_discovery;
using namespace web;
using boost::optional;

static char*
ConvertToCString(const std::string& str)
{
    char* cString = (char*)calloc(str.size() + 1, 1);
    if (cString) {
        std::memcpy(cString, str.data(), str.size());
    }
    return cString;
}

static std::shared_ptr<spdlog::logger> gLogger;
static void (*gSignalHandler)() = nullptr;

void
FromPitayaServer(CServer* server, const pitaya::Server& pServer)
{
    server->id = ConvertToCString(pServer.Id());
    server->type = ConvertToCString(pServer.Type());
    server->metadata = ConvertToCString(pServer.Metadata());
    server->hostname = ConvertToCString(pServer.Hostname());
    server->frontend = pServer.IsFrontend();
}

static void
FreePitayaError(CPitayaError* err)
{
    free(err->code);
    free(err->msg);
    // free(err);
}

static void
FreeServer(const CServer* sv)
{
    free(sv->id);
    free(sv->type);
    free(sv->metadata);
    free(sv->hostname);
}

static bool
ParseServerTypeFilters(std::vector<std::string>& serverTypeFilters, const char* serverTypeFiltersStr)
{
    serverTypeFilters = std::vector<std::string>();
    
    if (!serverTypeFiltersStr) {
        return true;
    }
    
    try {
        json::value val = json::value::parse(serverTypeFiltersStr);
        if (!val.is_array()) {
            gLogger->error("Server type filters should be a json array: {}", serverTypeFiltersStr);
            return false;
        }
        
        for (const auto& el : val.as_array()) {
            if (!el.is_string() || el.as_string().empty()) {
                gLogger->error("Server type filter elements should be of type string and not empty");
                return false;
            }
            serverTypeFilters.push_back(el.as_string());
        }
        
        return true;
    } catch (const json::json_exception& exc) {
        gLogger->error("Failed to parse serverTypeFilters: {}", exc.what());
        return false;
    }
}

class CServiceDiscoveryListener : public service_discovery::Listener
{
public:
    using ServerAddedOrRemovedCb = void (*)(int32_t serverAdded, CServer* server, void* user);

    CServiceDiscoveryListener(ServerAddedOrRemovedCb cb, void* user)
        : _onServerAddedOrRemoved(cb)
        , _user(user)
    {}

    void ServerAdded(const pitaya::Server& server) override
    {
        CServer cServer;
        FromPitayaServer(&cServer, server);

        _onServerAddedOrRemoved(true, &cServer, _user);

        FreeServer(&cServer);
    }

    void ServerRemoved(const pitaya::Server& server) override
    {
        CServer cServer;
        FromPitayaServer(&cServer, server);

        _onServerAddedOrRemoved(false, &cServer, _user);

        FreeServer(&cServer);
    }

private:
    ServerAddedOrRemovedCb _onServerAddedOrRemoved;
    void* _user;
};

bool
CSDConfig::TryGetConfig(pitaya::EtcdServiceDiscoveryConfig& config)
{
    config = pitaya::EtcdServiceDiscoveryConfig();
    config.endpoints = endpoints ? std::string(endpoints) : "";
    config.etcdPrefix = etcdPrefix ? std::string(etcdPrefix) : "";
    config.heartbeatTTLSec = std::chrono::seconds(heartbeatTTLSec);
    config.logHeartbeat = logHeartbeat;
    config.logServerSync = logServerSync;
    config.logServerDetails = logServerDetails;
    config.syncServersIntervalSec = std::chrono::seconds(syncServersIntervalSec);
    config.maxNumberOfRetries = maxNumberOfRetries;
    config.retryDelayMilliseconds = retryDelayMilliseconds;
    return ParseServerTypeFilters(config.serverTypeFilters, this->serverTypeFilters);
}

pitaya::GrpcConfig
CGrpcConfig::ToConfig()
{
    pitaya::GrpcConfig config;
    config.host = host;
    config.port = port;
    config.serverShutdownDeadline = std::chrono::milliseconds(serverShutdownDeadlineMs);
    config.serverMaxNumberOfRpcs = serverMaxNumberOfRpcs;
    config.clientRpcTimeout = std::chrono::milliseconds(clientRpcTimeoutMs);
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
        bindingStorageConfig.etcdPrefix = sdConfig->etcdPrefix;
        bindingStorageConfig.leaseTtlSec = 5;

        EtcdServiceDiscoveryConfig serviceDiscoveryConfig;
        if (!sdConfig->TryGetConfig(serviceDiscoveryConfig)) {
            return false;
        }

        try {
            Cluster::Instance().InitializeWithGrpc(grpcConfig->ToConfig(),
                                                   serviceDiscoveryConfig,
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
        using std::chrono::milliseconds;

        if (!spdlog::get("c_wrapper")) {
            gLogger = CreateLogger(logFile);
        }
        SetLogLevel(logLevel);
        auto natsCfg = NatsConfig(nc->addr ? std::string(nc->addr) : "",
                                  milliseconds(nc->requestTimeoutMs),
                                  milliseconds(nc->connectionTimeoutMs),
                                  milliseconds(nc->serverShutdownDeadlineMs),
                                  nc->serverMaxNumberOfRpcs,
                                  nc->maxReconnectionAttempts,
                                  nc->maxPendingMsgs,
                                  nc->reconnectWait,
                                  nc->reconnectBufSize);
        Server server = CServerToServer(sv);

        EtcdServiceDiscoveryConfig serviceDiscoveryConfig;
        if (!sdConfig->TryGetConfig(serviceDiscoveryConfig)) {
            return false;
        }

        try {
            Cluster::Instance().InitializeWithNats(std::move(natsCfg),
                                                   serviceDiscoveryConfig,
                                                   server,
                                                   "c_wrapper");
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

    void tfg_pitc_Terminate() { pitaya::Cluster::Instance().Terminate(); }

    static bool SendResponseToManaged(MemoryBuffer** outBuf,
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
                                 CPitayaError* retErr)
    {
        protos::Push push;

        bool success = push.ParseFromArray(memBuf->data, memBuf->size);
        if (!success) {
            retErr->code = ConvertToCString(pitaya::constants::kCodeInternalError);
            retErr->msg = ConvertToCString("failed to deserialize push");
            return false;
        }

        auto err = Cluster::Instance().SendPushToUser(server_id, server_type, push);

        if (err) {
            retErr->code = ConvertToCString(err->code);
            retErr->msg = ConvertToCString(err->msg);
            return false;
        }

        return true;
    }

    bool tfg_pitc_SendKickToUser(const char* serverId,
                                 const char* serverType,
                                 MemoryBuffer* memBuf,
                                 CPitayaError* retErr)
    {
        protos::KickMsg kick;

        bool success = kick.ParseFromArray(memBuf->data, memBuf->size);
        if (!success) {
            retErr->code = ConvertToCString(pitaya::constants::kCodeInternalError);
            retErr->msg = ConvertToCString("failed to deserialize kick msg");
            return false;
        }

        auto err = Cluster::Instance().SendKickToUser(serverId, serverType, kick);

        if (err) {
            retErr->code = ConvertToCString(err->code);
            retErr->msg = ConvertToCString(err->msg);
            return false;
        }

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
        } else {
            gLogger->debug("received message on RPC: {}", res.data());
        }

        return SendResponseToManaged(outBuf, res, retErr);
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

    void tfg_pitc_FinishRpcCall(MemoryBuffer* mb, CRpc* crpc)
    {
        auto rpc = reinterpret_cast<pitaya::Rpc*>(crpc->tag);

        protos::Response res;

        bool success = res.ParseFromArray(mb->data, mb->size);
        if (!success) {
            auto err = new protos::Error();
            err->set_code(pitaya::constants::kCodeInternalError);
            err->set_msg("pinvoke failed");
            res.set_allocated_error(err);
        }

        rpc->Finish(res);

        free(crpc->req->data);
        delete crpc->req;
        delete crpc;
    }

    CRpc* tfg_pitc_WaitForRpc()
    {
        boost::optional<Cluster::RpcData> rpcData = Cluster::Instance().WaitForRpc();

        if (!rpcData) {
            // There are no RPCs left
            return nullptr;
        }

        MemoryBuffer* reqBuffer = new MemoryBuffer();
        size_t size = rpcData->req.ByteSizeLong();
        reqBuffer->data = malloc(size);
        reqBuffer->size = size;

        bool success = rpcData->req.SerializeToArray(reqBuffer->data, size);
        if (!success) {
            // TODO: send in response?
            gLogger->error("failed to serialize protobuf request!");
        }

        CRpc* crpc = new CRpc();
        crpc->req = reqBuffer;
        crpc->tag = rpcData->rpc;

        return crpc;
    }

    void* tfg_pitc_AddServiceDiscoveryListener(CServiceDiscoveryListener::ServerAddedOrRemovedCb cb,
                                               void* user)
    {
        gLogger->info("Adding native service discovery listener");
        auto listener = new CServiceDiscoveryListener(cb, user);
        Cluster::Instance().AddServiceDiscoveryListener(listener);
        return listener;
    }

    void tfg_pitc_RemoveServiceDiscoveryListener(service_discovery::Listener* listener)
    {
        gLogger->info("Removing native service discovery listener");
        if (listener) {
            Cluster::Instance().RemoveServiceDiscoveryListener(listener);
            delete listener;
        } else {
            gLogger->warn("Received a null listener");
        }
    }
}
