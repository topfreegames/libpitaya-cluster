#include "pitaya.h"
#include "pitaya/cluster.h"

#include <chrono>
#include <string>

struct CPitayaError
{
    char* code = nullptr;
    char* msg = nullptr;
};

struct CServer
{
    char* id = nullptr;
    char* type = nullptr;
    char* metadata = nullptr;
    char* hostname = nullptr;
    int32_t frontend = false;
};

enum LogLevel : int
{
    LogLevel_Debug = 0,
    LogLevel_Info = 1,
    LogLevel_Warn = 2,
    LogLevel_Error = 3,
    LogLevel_Critical = 4,
};

struct CSDConfig
{
    const char* endpoints;
    const char* etcdPrefix;
    const char* serverTypeFilters;
    int32_t heartbeatTTLSec;
    int32_t logHeartbeat;
    int32_t logServerSync;
    int32_t logServerDetails;
    int32_t syncServersIntervalSec;
    int32_t maxNumberOfRetries = 5;
    int32_t retryDelayMilliseconds;

    bool TryGetConfig(pitaya::EtcdServiceDiscoveryConfig& config);
};

struct CGrpcConfig
{
    const char* host;
    int32_t port;
    int32_t serverShutdownDeadlineMs;
    int32_t serverMaxNumberOfRpcs;
    int32_t clientRpcTimeoutMs;

    pitaya::GrpcConfig ToConfig();
};

struct CNATSConfig
{
    const char* addr;
    int64_t connectionTimeoutMs;
    int32_t requestTimeoutMs;
    int32_t serverShutdownDeadlineMs;
    int32_t serverMaxNumberOfRpcs;
    int32_t maxReconnectionAttempts;
    int32_t maxPendingMsgs;
    int32_t reconnectBufSize;
    int64_t reconnectWait = 2000;
};

struct CBindingStorageConfig
{
    int32_t leaseTtlSec;
    const char* endpoint;
    const char* etcdPrefix;

    pitaya::EtcdBindingStorageConfig ToConfig();
};

extern "C"
{
    struct MemoryBuffer
    {
        void* data;
        int size;
    };

    typedef void (*CsharpFreeCb)(void*);
    typedef MemoryBuffer* (*RpcPinvokeCb)(MemoryBuffer*);

    bool tfg_pitc_InitializeWithGrpc(CGrpcConfig* grpcConfig,
                                     CSDConfig* sdConfig,
                                     CServer* sv,
                                     LogLevel logLevel,
                                     const char* logFile);

    bool tfg_pitc_InitializeWithNats(CNATSConfig* nc,
                                     CSDConfig* sdConfig,
                                     CServer* sv,
                                     LogLevel logLevel,
                                     const char* logFile);

    bool tfg_pitc_GetServerById(const char* serverId, CServer* retServer);

    void tfg_pitc_FreeServer(CServer* cServer);

    void tfg_pitc_Terminate();

    bool tfg_pitc_RPC(const char* serverId,
                      const char* route,
                      void* data,
                      int dataSize,
                      MemoryBuffer** outBuf,
                      CPitayaError* retErr);

    bool tfg_pitc_SendKickToUser(const char* serverId,
                                 const char* serverType,
                                 MemoryBuffer* memBuf,
                                 CPitayaError* retErr);

    bool tfg_pitc_SendPushToUser(const char* server_id,
                                 const char* server_type,
                                 MemoryBuffer* memBuf,
                                 CPitayaError* retErr);
}
