using System;
using System.Runtime.InteropServices;
using Google.Protobuf;
using System.Collections.Generic;
using NPitaya.Models;
using NPitaya.Serializer;
using Protos;
using static NPitaya.Utils.Utils;

// TODO remove try catches
// TODO json support
namespace NPitaya
{
    public partial class PitayaCluster
    {
        private static ISerializer serializer = new ProtobufSerializer();
        public delegate string RemoteNameFunc(string methodName);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate void FreeHGlobalDelegate(IntPtr ptr);

        static readonly FreeHGlobalDelegate FreeDelegate = Marshal.FreeHGlobal;
        private static readonly IntPtr FreeHGlobalPtr = Marshal.GetFunctionPointerForDelegate(FreeDelegate);

        private delegate void OnSignalFunc();

        private static readonly Dictionary<string, RemoteMethod> RemotesDict = new Dictionary<string, RemoteMethod>();
        private static readonly Dictionary<string, RemoteMethod> HandlersDict = new Dictionary<string, RemoteMethod>();

        private static Action _onSignalEvent;

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

        public static void Initialize(GrpcConfig grpcCfg, SDConfig sdCfg, Server server, NativeLogLevel logLevel,
            string logFile = "")
        {
            IntPtr grpcCfgPtr = new StructWrapper(grpcCfg);
            IntPtr sdCfgPtr = new StructWrapper(sdCfg);
            IntPtr serverPtr = new StructWrapper(server);

            bool ok = InitializeWithGrpcInternal(grpcCfgPtr, sdCfgPtr, serverPtr, RpcCbFuncPtr, FreeHGlobalPtr,
                logLevel,
                logFile);

            if (!ok)
            {
                throw new PitayaException("Initialization failed");
            }
        }

        public static void Initialize(NatsConfig natsCfg, SDConfig sdCfg, Server server, NativeLogLevel logLevel,
            string logFile = "")
        {
            IntPtr natsCfgPtr = new StructWrapper(natsCfg);
            IntPtr sdCfgPtr = new StructWrapper(sdCfg);
            IntPtr serverPtr = new StructWrapper(server);

            bool ok = InitializeWithNatsInternal(natsCfgPtr, sdCfgPtr, serverPtr, RpcCbFuncPtr, FreeHGlobalPtr,
                logLevel,
                logFile);

            if (!ok)
            {
                throw new PitayaException("Initialization failed");
            }
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

        public static void
            RegisterRemote(BaseRemote remote, string name,
                RemoteNameFunc remoteNameFunc) // TODO remote function name func
        {
            Dictionary<string, RemoteMethod> m = remote.getRemotesMap();
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
            Dictionary<string, RemoteMethod> m = handler.getRemotesMap();
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
            serializer = s;
        }

        public static void Terminate()
        {
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

        public static unsafe bool SendPushToUser(string frontendId, string serverType, string route, string uid, object pushMsg)
        {
            bool ok = false;
            MemoryBuffer inMemBuf = new MemoryBuffer();
            MemoryBuffer* outMemBufPtr = null;
            var retError = new Error();

            var push = new Push
            {
                Route = route,
                Uid = uid,
                Data = ByteString.CopyFrom(SerializerUtils.SerializeOrRaw(pushMsg, serializer))
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
            bool ok = false;
            MemoryBuffer* memBufPtr = null;
            var retError = new Error();

            try
            {
                var data = SerializerUtils.SerializeOrRaw(msg, serializer);
                fixed (byte* p = data)
                {
                    ok = RPCInternal(serverId, route.ToString(), (IntPtr) p, data.Length, &memBufPtr, ref retError);
                }

                if (!ok) // error
                {
                    throw new PitayaException($"RPC call failed: ({retError.code}: {retError.msg})");
                }

                var protoRet = GetProtoMessageFromMemoryBuffer<T>(*memBufPtr);
                return protoRet;
            }
            finally
            {
                if (memBufPtr != null) FreeMemoryBufferInternal(memBufPtr);
            }
        }

        public static T Rpc<T>(Route route, IMessage msg)
        {
            return Rpc<T>("", route, msg);
        }
    }
}