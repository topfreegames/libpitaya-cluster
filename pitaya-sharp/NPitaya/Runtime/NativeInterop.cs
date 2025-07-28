using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using PitayaSimpleJson;

namespace NPitaya
{

    [StructLayout(LayoutKind.Sequential)]
    public struct CRpc
    {
        public IntPtr reqBufferPtr;
        public IntPtr tag;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Server
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string id;
        [MarshalAs(UnmanagedType.LPStr)]
        public string type;
        [MarshalAs(UnmanagedType.LPStr)]
        public string metadata;
        [MarshalAs(UnmanagedType.LPStr)]
        public string hostname;

        public int frontend;

        public Server(string id, string type, string metadata, string hostname, bool frontend)
        {
            this.id = id;
            this.type = type;
            this.metadata = metadata;
            this.hostname = hostname;
            this.frontend = Convert.ToInt32(frontend);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Error
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string code;
        [MarshalAs(UnmanagedType.LPStr)]
        public string msg;
    }

    public enum NativeLogLevel
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Critical = 4,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SDConfig
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string endpoints;
        [MarshalAs(UnmanagedType.LPStr)]
        public string etcdPrefix;
        [MarshalAs(UnmanagedType.LPStr)]
        public string serverTypeFiltersStr;
        public int heartbeatTTLSec;
        public int logHeartbeat;
        public int logServerSync;
        public int logServerDetails;
        public int syncServersIntervalSec;
        public int maxNumberOfRetries;
        public int retryDelayMilliseconds;

        public SDConfig(string endpoints, string etcdPrefix, List<string> serverTypeFilters, int heartbeatTTLSec, bool logHeartbeat,
            bool logServerSync, bool logServerDetails, int syncServersIntervalSec, int maxNumberOfRetries, int retryDelayMilliseconds)
        {
            this.endpoints = endpoints;
            this.etcdPrefix = etcdPrefix;
            this.heartbeatTTLSec = heartbeatTTLSec;
            this.logHeartbeat = Convert.ToInt32(logHeartbeat);
            this.logServerSync = Convert.ToInt32(logServerSync);
            this.logServerDetails = Convert.ToInt32(logServerDetails);
            this.syncServersIntervalSec = syncServersIntervalSec;
            this.maxNumberOfRetries = maxNumberOfRetries;
            this.retryDelayMilliseconds = retryDelayMilliseconds;
            try
            {
                serverTypeFiltersStr = SimpleJson.SerializeObject(serverTypeFilters);
            }
            catch (Exception e)
            {
                throw new Exception("Failed to serialize serverTypeFilters: " + e.Message);
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GrpcConfig
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string host;
        public int port;
        public int serverShutdownDeadlineMs;
        public int serverMaxNumberOfRpcs;
        public int clientRpcTimeoutMs;

        public GrpcConfig(
            string host,
            int port,
            int serverShutdownDeadlineMs,
            int serverMaxNumberOfRpcs,
            int clientRpcTimeoutMs)
        {
            this.host = host;
            this.port = port;
            this.serverShutdownDeadlineMs = serverShutdownDeadlineMs;
            this.serverMaxNumberOfRpcs = serverMaxNumberOfRpcs;
            this.clientRpcTimeoutMs = clientRpcTimeoutMs;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Route
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string svType;
        [MarshalAs(UnmanagedType.LPStr)]
        public string service;
        [MarshalAs(UnmanagedType.LPStr)]
        public string method;

        public Route(string svType, string service, string method):this(service, method)
        {
            this.svType = svType;
        }

        public Route(string service, string method)
        {
            this.service = service;
            this.method = method;
            svType = "";
        }

        public static Route FromString(string r)
        {
            string[] res = r.Split(new[] { "." }, StringSplitOptions.None);
            if (res.Length == 3)
            {
                return new Route(res[0], res[1], res[2]);
            }
            if (res.Length == 2)
            {
                return new Route(res[0], res[1]);
            }
            throw new Exception($"invalid route: {r}");
        }

        public override string ToString()
        {
            if (svType.Length > 0)
            {
                return $"{svType}.{service}.{method}";
            }
            return $"{service}.{method}";
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MemoryBuffer
    {
        public IntPtr data;
        public int size;

        public byte[] GetData()
        {
            byte[] data = new byte[this.size];
            Marshal.Copy(this.data, data, 0, this.size);
            return data;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NatsConfig
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string endpoint;
        public Int64 connectionTimeoutMs;
        public int requestTimeoutMs;
        public int serverShutdownDeadlineMs;
        public int serverMaxNumberOfRpcs;
        public int maxReconnectionAttempts;
        public int maxPendingMessages;
        public int reconnectBufSize;
        public Int64 reconnectWaitInMs;
        public Int64 reconnectJitterInMs;
        public Int64 pingIntervalInMs;
        public int maxPingsOut;

        // Backward compatibility constructor with default values
        [Obsolete("This constructor is deprecated. Use the constructor without maxConnectionRetries parameter or NatsConfig.CreateWithDefaults().")]
        public NatsConfig(string endpoint,
                          int connectionTimeoutMs,
                          int requestTimeoutMs,
                          int serverShutdownDeadlineMs,
                          int serverMaxNumberOfRpcs,
                          int maxConnectionRetries, // Deprecated parameter - ignored
                          int maxPendingMessages,
                          int reconnectBufSize,
                          int maxReconnectionAttempts = 30,
                          int reconnectWaitInMs = 100,
                          int reconnectJitterInMs = 50,
                          int pingIntervalInMs = 1000,
                          int maxPingsOut = 2)
        {
            this.endpoint = endpoint;
            this.connectionTimeoutMs = connectionTimeoutMs;
            this.requestTimeoutMs = requestTimeoutMs;
            this.serverShutdownDeadlineMs = serverShutdownDeadlineMs;
            this.serverMaxNumberOfRpcs = serverMaxNumberOfRpcs;
            this.maxReconnectionAttempts = maxReconnectionAttempts;
            this.maxPendingMessages = maxPendingMessages;
            this.reconnectBufSize = reconnectBufSize;
            this.reconnectWaitInMs = reconnectWaitInMs;
            this.reconnectJitterInMs = reconnectJitterInMs;
            this.pingIntervalInMs = pingIntervalInMs;
            this.maxPingsOut = maxPingsOut;
        }

        // Simple constructor for common use case
        public NatsConfig(string endpoint)
        {
            this.endpoint = endpoint;
            this.connectionTimeoutMs = 2000;
            this.requestTimeoutMs = 2000;
            this.serverShutdownDeadlineMs = 2000;
            this.serverMaxNumberOfRpcs = 500;
            this.maxReconnectionAttempts = 30;
            this.maxPendingMessages = 100;
            this.reconnectBufSize = 8 * 1024 * 1024;
            this.reconnectWaitInMs = 100;
            this.reconnectJitterInMs = 50;
            this.pingIntervalInMs = 1000;
            this.maxPingsOut = 2;
        }

        // New constructor without deprecated maxConnectionRetries parameter
        public NatsConfig(string endpoint,
                          int connectionTimeoutMs,
                          int requestTimeoutMs,
                          int serverShutdownDeadlineMs,
                          int serverMaxNumberOfRpcs,
                          int maxPendingMessages,
                          int reconnectBufSize,
                          int maxReconnectionAttempts = 30,
                          int reconnectWaitInMs = 100,
                          int reconnectJitterInMs = 50,
                          int pingIntervalInMs = 1000,
                          int maxPingsOut = 2)
        {
            this.endpoint = endpoint;
            this.connectionTimeoutMs = connectionTimeoutMs;
            this.requestTimeoutMs = requestTimeoutMs;
            this.serverShutdownDeadlineMs = serverShutdownDeadlineMs;
            this.serverMaxNumberOfRpcs = serverMaxNumberOfRpcs;
            this.maxReconnectionAttempts = maxReconnectionAttempts;
            this.maxPendingMessages = maxPendingMessages;
            this.reconnectBufSize = reconnectBufSize;
            this.reconnectWaitInMs = reconnectWaitInMs;
            this.reconnectJitterInMs = reconnectJitterInMs;
            this.pingIntervalInMs = pingIntervalInMs;
            this.maxPingsOut = maxPingsOut;
        }

        // Static factory method for backward compatibility
        public static NatsConfig CreateWithDefaults(string endpoint)
        {
            return new NatsConfig(
                endpoint,
                2000, // connectionTimeoutMs
                2000, // requestTimeoutMs
                2000, // serverShutdownDeadlineMs
                500,  // serverMaxNumberOfRpcs
                100,  // maxPendingMessages
                8 * 1024 * 1024, // reconnectBufSize
                30,   // maxReconnectionAttempts
                100, // reconnectWaitInMs
                50,   // reconnectJitterInMs
                1000, // pingIntervalInMs
                2     // maxPingsOut
            );
        }
    }
}

class StructWrapper : IDisposable
{
    public IntPtr Ptr { get; private set; }

    public StructWrapper(object obj)
    {
        Ptr = Marshal.AllocHGlobal(Marshal.SizeOf(obj));
        Marshal.StructureToPtr(obj, Ptr, false);
    }

    ~StructWrapper()
    {
        if (Ptr != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(Ptr);
            Ptr = IntPtr.Zero;
        }
    }

    public void Dispose()
    {
        Marshal.FreeHGlobal(Ptr);
        Ptr = IntPtr.Zero;
        GC.SuppressFinalize(this);
    }

    public static implicit operator IntPtr(StructWrapper w)
    {
        return w.Ptr;
    }
}
