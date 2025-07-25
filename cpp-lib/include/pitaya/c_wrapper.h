#include "pitaya.h"
#include "pitaya/cluster.h"
#include "pitaya/nats_config.h"

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
    int64_t connectionTimeoutMs = PITAYA_NATS_DEFAULT_CONNECTION_TIMEOUT_IN_MS;
    int32_t requestTimeoutMs = PITAYA_NATS_DEFAULT_REQUEST_TIMEOUT_IN_MS;
    int32_t serverShutdownDeadlineMs = PITAYA_NATS_DEFAULT_SERVER_SHUTDOWN_DEADLINE_IN_MS;
    int32_t serverMaxNumberOfRpcs = PITAYA_NATS_DEFAULT_SERVER_MAX_NUMBER_OF_RPCS;
    int32_t maxReconnectionAttempts = PITAYA_NATS_DEFAULT_MAX_RECONNECTION_ATTEMPTS;
    int32_t maxPendingMsgs = PITAYA_NATS_DEFAULT_MAX_PENDING_MSGS;
    int32_t reconnectBufSize = PITAYA_NATS_DEFAULT_RECONNECT_BUF_SIZE;
    int64_t reconnectWaitInMs = PITAYA_NATS_DEFAULT_RECONNECT_WAIT_IN_MS;
    int64_t reconnectJitterInMs = PITAYA_NATS_DEFAULT_RECONNECT_JITTER_IN_MS;
    int64_t pingIntervalInMs = PITAYA_NATS_DEFAULT_PING_INTERVAL_IN_MS;
    int32_t maxPingsOut = PITAYA_NATS_DEFAULT_MAX_PINGS_OUT;
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
