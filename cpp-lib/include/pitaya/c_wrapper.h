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
    int frontend = false;
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
    int heartbeatTTLSec;
    int logHeartbeat;
    int logServerSync;
    int logServerDetails;
    int syncServersIntervalSec;

    pitaya::etcdv3_service_discovery::Config ToConfig();
};

struct CGrpcConfig
{
    const char* host;
    int port;
    int connectionTimeoutSec;

    pitaya::GrpcConfig ToConfig();
};

struct CNATSConfig
{
    const char* addr;
    int64_t connectionTimeoutMs;
    int requestTimeoutMs;
    int maxReconnectionAttempts;
    int maxPendingMsgs;
};

struct CBindingStorageConfig
{
    int leaseTtlSec;
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
