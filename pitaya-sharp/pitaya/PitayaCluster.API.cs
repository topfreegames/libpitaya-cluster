using System;
using System.Runtime.InteropServices;
using Google.Protobuf;
using System.Collections.Generic;
using Pitaya.Models;
using Protos;
using static Pitaya.Utils.Utils;

// TODO remove try catches
// TODO json support
namespace Pitaya
{
    public partial class PitayaCluster
    {

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
      
            bool ok = InitializeWithGrpcInternal(grpcCfgPtr, sdCfgPtr, serverPtr, RpcCbFuncPtr, FreeHGlobalPtr, logLevel,
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

            bool ok = InitializeWithNatsInternal(natsCfgPtr, sdCfgPtr, serverPtr, RpcCbFuncPtr, FreeHGlobalPtr, logLevel,
                logFile);

            if (!ok)
            {
                throw new PitayaException("Initialization failed");
            }
        }

        public static void RegisterRemote(BaseRemoteMethod remoteMethod)
        {
            string className = DefaultRemoteNameFunc(remoteMethod.GetName());
            RegisterRemote(remoteMethod, className, DefaultRemoteNameFunc);
        }

        public static void RegisterRemote(BaseRemoteMethod remoteMethod, string name)
        {
            RegisterRemote(remoteMethod, name, DefaultRemoteNameFunc);
        }

        public static void RegisterRemote(BaseRemoteMethod remoteMethod, string name, RemoteNameFunc remoteNameFunc) // TODO remote function name func
        {
            Dictionary<string, RemoteMethod> m = remoteMethod.getRemotesMap();
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
    
        public static void RegisterHandler(BaseHandlerMethod handlerMethod)
        {
            string className = DefaultRemoteNameFunc(handlerMethod.GetName());
            RegisterHandler(handlerMethod, className, DefaultRemoteNameFunc);
        }

        public static void RegisterHandler(BaseHandlerMethod handlerMethod, string name)
        {
            RegisterHandler(handlerMethod, name, DefaultRemoteNameFunc);
        }

        public static void RegisterHandler(BaseHandlerMethod handlerMethod, string name, RemoteNameFunc remoteNameFunc)
        {
            Dictionary<string, RemoteMethod> m = handlerMethod.getRemotesMap();
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

        public static unsafe bool SendPushToUser(string frontendId, string serverType, Push push)
        {
            bool ok = false;
            MemoryBuffer inMemBuf = new MemoryBuffer();
            MemoryBuffer* outMemBufPtr = null;
            var retError = new Error();

            try{
                var data = ProtoMessageToByteArray(push);
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
                var data = ProtoMessageToByteArray(kick);
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
        public static unsafe T Rpc<T>(string serverId, Route route, IMessage msg)
        {
            bool ok = false;
            MemoryBuffer* memBufPtr = null;
            var retError = new Error();

            try
            {
                var data = ProtoMessageToByteArray(msg);
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
                if(memBufPtr != null) FreeMemoryBufferInternal(memBufPtr);
            }
        }

        public static T Rpc<T>(Route route, IMessage msg)
        {
            return Rpc<T>("", route, msg);
        }
    }
}