using System;
using Google.Protobuf;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using NPitaya.Metrics;
using NPitaya.Models;
using NPitaya.Serializer;
using NPitaya.Protos;
using static NPitaya.Utils.Utils;

// TODO profiling
// TODO better reflection performance in task async call
// TODO support to sync methods
namespace NPitaya
{
    public partial class PitayaCluster
    {
        private static readonly int ProcessorsCount = Environment.ProcessorCount;
        private static ISerializer _serializer = new ProtobufSerializer();

        public delegate string RemoteNameFunc(string methodName);

        private delegate void OnSignalFunc();

        private static readonly Dictionary<string, RemoteMethod> RemotesDict = new Dictionary<string, RemoteMethod>();
        private static readonly Dictionary<string, RemoteMethod> HandlersDict = new Dictionary<string, RemoteMethod>();

        private static Action _onSignalEvent;

        public enum ServiceDiscoveryAction
        {
            ServerAdded,
            ServerRemoved,
        }

        public class ServiceDiscoveryListener
        {
            public Action<ServiceDiscoveryAction, Server> onServer;
            public IntPtr NativeListenerHandle { get; set; }
            public ServiceDiscoveryListener(Action<ServiceDiscoveryAction, Server> onServer)
            {
                Debug.Assert(onServer != null);
                this.onServer = onServer;
                NativeListenerHandle = IntPtr.Zero;
            }
        }

        private static ServiceDiscoveryListener _serviceDiscoveryListener;
        private static GCHandle _serviceDiscoveryListenerHandle;

        public static void AddSignalHandler(Action cb)
        {
            _onSignalEvent += cb;
            OnSignalInternal(OnSignal);
        }

        private static void OnSignal()
        {
            Logger.Info("Invoking signal handler");
            _onSignalEvent?.Invoke();
        }

        public static void Initialize(GrpcConfig grpcCfg,
                                      SDConfig sdCfg,
                                      Server server,
                                      NativeLogLevel logLevel,
                                      ServiceDiscoveryListener serviceDiscoveryListener = null,
                                      string logFile = "")
        {
            IntPtr grpcCfgPtr = new StructWrapper(grpcCfg);
            IntPtr sdCfgPtr = new StructWrapper(sdCfg);
            IntPtr serverPtr = new StructWrapper(server);

            bool ok = InitializeWithGrpcInternal(grpcCfgPtr, sdCfgPtr, serverPtr, logLevel,
                logFile);

            if (!ok)
            {
                throw new PitayaException("Initialization failed");
            }

            AddServiceDiscoveryListener(serviceDiscoveryListener);
            ListenToIncomingRPCs();
        }

        private static void ListenToIncomingRPCs()
        {
            for (int i = 0; i < ProcessorsCount; i++)
            {
                var threadId = i + 1;
                new Thread(() =>
                {
                    Logger.Debug($"[Consumer thread {threadId}] Started");
                    for (;;)
                    {
                        var cRpcPtr = tfg_pitc_WaitForRpc();
                        if (cRpcPtr == IntPtr.Zero)
                        {
                            Logger.Debug($"[Consumer thread {threadId}] No more incoming RPCs, exiting");
                            break;
                        }
#pragma warning disable 4014
                        HandleIncomingRpc(cRpcPtr);
#pragma warning restore 4014
                    }
                }).Start();
            }
        }

        public static void Initialize(NatsConfig natsCfg,
                                      SDConfig sdCfg,
                                      Server server,
                                      NativeLogLevel logLevel,
                                      ServiceDiscoveryListener serviceDiscoveryListener = null,
                                      string logFile = "")
        {
            IntPtr natsCfgPtr = new StructWrapper(natsCfg);
            IntPtr sdCfgPtr = new StructWrapper(sdCfg);
            IntPtr serverPtr = new StructWrapper(server);

            bool ok = InitializeWithNatsInternal(natsCfgPtr, sdCfgPtr, serverPtr,
                logLevel,
                logFile);

            if (!ok)
            {
                throw new PitayaException("Initialization failed");
            }

            AddServiceDiscoveryListener(serviceDiscoveryListener);
            ListenToIncomingRPCs();
        }

        public static void RegisterRemote(BaseRemote remote)
        {
            string className = DefaultRemoteNameFunc(remote.GetName());
            RegisterRemote(remote, className, DefaultRemoteNameFunc);
        }

        public static void RegisterRemote(BaseRemote remote, string name)
        {
            RegisterRemote(remote, name, DefaultRemoteNameFunc);
        }

        public static void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc) // TODO remote function name func
        {
            Dictionary<string, RemoteMethod> m = remote.GetRemotesMap();
            foreach (KeyValuePair<string, RemoteMethod> kvp in m)
            {
                var rn = remoteNameFunc(kvp.Key);
                var remoteName = $"{name}.{rn}";
                if (RemotesDict.ContainsKey(remoteName))
                {
                    throw new PitayaException($"tried to register same remote twice! remote name: {remoteName}");
                }

                Logger.Info("registering remote {0}", remoteName);
                RemotesDict[remoteName] = kvp.Value;
            }
        }

        public static void RegisterHandler(BaseHandler handler)
        {
            string className = DefaultRemoteNameFunc(handler.GetName());
            RegisterHandler(handler, className, DefaultRemoteNameFunc);
        }

        public static void RegisterHandler(BaseHandler handler, string name)
        {
            RegisterHandler(handler, name, DefaultRemoteNameFunc);
        }

        public static void RegisterHandler(BaseHandler handler, string name, RemoteNameFunc remoteNameFunc)
        {
            Dictionary<string, RemoteMethod> m = handler.GetRemotesMap();
            foreach (KeyValuePair<string, RemoteMethod> kvp in m)
            {
                var rn = remoteNameFunc(kvp.Key);
                var handlerName = $"{name}.{rn}";
                if (HandlersDict.ContainsKey(handlerName))
                {
                    throw new PitayaException($"tried to register same remote twice! remote name: {handlerName}");
                }

                Logger.Info("registering handler {0}", handlerName);
                HandlersDict[handlerName] = kvp.Value;
            }
        }

        public static void SetSerializer(ISerializer s)
        {
            _serializer = s;
        }

        public static void Terminate()
        {
            RemoveServiceDiscoveryListener(_serviceDiscoveryListener);
            TerminateInternal();
        }

        public static Server? GetServerById(string serverId)
        {
            var retServer = new Server();

            bool ok = GetServerByIdInternal(serverId, ref retServer);

            if (!ok)
            {
                Logger.Error($"There are no servers with id {serverId}");
                return null;
            }

            //var server = (Pitaya.Server)Marshal.PtrToStructure(serverPtr, typeof(Pitaya.Server));
            //FreeServerInternal(serverPtr);
            return retServer;
        }

        public static unsafe bool SendPushToUser(string frontendId, string serverType, string route, string uid,
            object pushMsg)
        {
            bool ok = false;
            MemoryBuffer inMemBuf = new MemoryBuffer();
            MemoryBuffer* outMemBufPtr = null;
            var retError = new Error();

            var push = new Push
            {
                Route = route,
                Uid = uid,
                Data = ByteString.CopyFrom(SerializerUtils.SerializeOrRaw(pushMsg, _serializer))
            };

            try
            {
                var data = push.ToByteArray();
                fixed (byte* p = data)
                {
                    inMemBuf.data = (IntPtr) p;
                    inMemBuf.size = data.Length;
                    IntPtr inMemBufPtr = new StructWrapper(inMemBuf);

                    ok = PushInternal(frontendId, serverType, inMemBufPtr, &outMemBufPtr, ref retError);
                    if (!ok) // error
                    {
                        Logger.Error($"Push failed: ({retError.code}: {retError.msg})");
                        return false;
                    }

                    return true;
                }
            }
            finally
            {
                if (outMemBufPtr != null) FreeMemoryBufferInternal(outMemBufPtr);
            }
        }

        public static unsafe bool SendKickToUser(string frontendId, string serverType, KickMsg kick)
        {
            bool ok = false;
            MemoryBuffer inMemBuf = new MemoryBuffer();
            MemoryBuffer* outMemBufPtr = null;
            var retError = new Error();

            try
            {
                var data = kick.ToByteArray();
                fixed (byte* p = data)
                {
                    inMemBuf.data = (IntPtr) p;
                    inMemBuf.size = data.Length;
                    IntPtr inMemBufPtr = new StructWrapper(inMemBuf);
                    ok = KickInternal(frontendId, serverType, inMemBufPtr, &outMemBufPtr, ref retError);
                    if (!ok) // error
                    {
                        Logger.Error($"Push failed: ({retError.code}: {retError.msg})");
                        return false;
                    }

                    var kickAns = new KickAnswer();
                    kickAns.MergeFrom(new CodedInputStream(outMemBufPtr->GetData()));

                    return kickAns.Kicked;
                }
            }
            finally
            {
                if (outMemBufPtr != null) FreeMemoryBufferInternal(outMemBufPtr);
            }
        }

        public static unsafe T Rpc<T>(string serverId, Route route, object msg)
        {
            MemoryBuffer* memBufPtr = null;
            var retError = new Error();
            var ok = false;
            Stopwatch sw = null;
            try
            {
                var data = SerializerUtils.SerializeOrRaw(msg, _serializer);
                sw = Stopwatch.StartNew();
                fixed (byte* p = data)
                {
                    ok = RPCInternal(serverId, route.ToString(), (IntPtr) p, data.Length, &memBufPtr, ref retError);
                }
                sw.Stop();

                if (!ok) // error
                {
                    throw new PitayaException($"RPC call failed: ({retError.code}: {retError.msg})");
                }

                var protoRet = GetProtoMessageFromMemoryBuffer<T>(*memBufPtr);
                return protoRet;
            }
            finally
            {
                var status = ok ? Metrics.Constants.Status.success.ToString() : Metrics.Constants.Status.fail.ToString();
                if (sw != null) MetricsReporters.ReportTimer(status, route.ToString(), "rpc", retError.code, sw);
                if (memBufPtr != null) FreeMemoryBufferInternal(memBufPtr);
            }
        }

        public static T Rpc<T>(Route route, IMessage msg)
        {
            return Rpc<T>("", route, msg);
        }

        private static void OnServerAddedOrRemovedNativeCb(int serverAdded, IntPtr serverPtr, IntPtr user)
        {
            var pitayaClusterHandle = (GCHandle)user;
            var serviceDiscoveryListener = pitayaClusterHandle.Target as ServiceDiscoveryListener;

            if (serviceDiscoveryListener == null)
            {
                Logger.Warn("The service discovery listener is null!");
                return;
            }

            var server = (Server)Marshal.PtrToStructure(serverPtr, typeof(Server));

            if (serverAdded == 1)
                serviceDiscoveryListener.onServer(ServiceDiscoveryAction.ServerAdded, server);
            else
                serviceDiscoveryListener.onServer(ServiceDiscoveryAction.ServerRemoved, server);
        }

        private static void AddServiceDiscoveryListener(ServiceDiscoveryListener listener)
        {
            _serviceDiscoveryListener = listener;
            if (listener == null)
                return;

            _serviceDiscoveryListenerHandle = GCHandle.Alloc(_serviceDiscoveryListener);

            IntPtr nativeListenerHandle = tfg_pitc_AddServiceDiscoveryListener(
                OnServerAddedOrRemovedNativeCb,
                (IntPtr)_serviceDiscoveryListenerHandle
            );

            listener.NativeListenerHandle = nativeListenerHandle;
        }

        private static void RemoveServiceDiscoveryListener(ServiceDiscoveryListener listener)
        {
            if (listener != null)
            {
                tfg_pitc_RemoveServiceDiscoveryListener(listener.NativeListenerHandle);
                _serviceDiscoveryListenerHandle.Free();
                _serviceDiscoveryListener = null;
            }
        }
    }
}