extern "C"
{
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

        static CServer* FromPitayaServer(const pitaya::Server& pServer);
    };

    struct CPitayaError
    {
        char* code = nullptr;
        char* msg = nullptr;

        CPitayaError(const std::string& codeStr, const std::string& msgStr);

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
        LogLevel logLevel;

        pitaya::etcdv3_service_discovery::Config ToConfig()
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
    };

    struct CNATSConfig
    {
        const char* addr;
        int64_t connectionTimeoutMs;
        int requestTimeoutMs;
        int maxReconnectionAttempts;
        int maxPendingMsgs;
    };

    typedef void (*CsharpFreeCb)(void*);
    typedef MemoryBuffer* (*RpcPinvokeCb)(RPCReq*);

    bool tfg_pitc_Initialize(CServer* sv,
                             CSDConfig* sdConfig,
                             CNATSConfig* nc,
                             RpcPinvokeCb cb,
                             CsharpFreeCb freeCb,
                             const char* logFile);

    CServer* tfg_pitc_GetServerById(const char* serverId);

    void tfg_pitc_FreeServer(CServer* cServer);

    void tfg_pitc_Terminate();

    CPitayaError* tfg_pitc_RPC(const char* serverId,
                               const char* route,
                               void* data,
                               int dataSize,
                               MemoryBuffer** outBuf);
}
