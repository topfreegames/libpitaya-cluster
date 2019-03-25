using System;
using System.IO;
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
  public class PitayaCluster
  {
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate IntPtr RPCCb(IntPtr req);

    public delegate string RemoteNameFunc(string methodName);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void FreeHGlobalDelegate(IntPtr ptr);
    
    static readonly FreeHGlobalDelegate FreeDelegate = Marshal.FreeHGlobal;
    private static readonly IntPtr FreeHGlobalPtr = Marshal.GetFunctionPointerForDelegate(FreeDelegate);
    
    static readonly RPCCb RpcCbFunc = RPCCbFunc;
    private static readonly IntPtr RpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(RpcCbFunc);
      
    private delegate void OnSignalFunc();

    private static readonly Dictionary<string, RemoteMethod> RemotesDict = new Dictionary<string, RemoteMethod>();
    private static readonly Dictionary<string, RemoteMethod> HandlersDict = new Dictionary<string, RemoteMethod>();

    private static Action _onSignalEvent;

    private static Protos.Response GetErrorResponse(string code, string msg)
    {
      var response = new Protos.Response {Error = new Protos.Error {Code = code, Msg = msg}};
      return response;
    }

    private static byte[] ProtoMessageToByteArray(IMessage msg)
    {
      if (msg == null)
      {
        return new byte[] { };
      }

      var mem = new MemoryStream();
      var o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      return mem.ToArray();
    }

    private static IntPtr ByteArrayToIntPtr(byte[] data)
    {
      IntPtr ptr = Marshal.AllocHGlobal(data.Length);
      Marshal.Copy(data, 0, ptr, data.Length);
      return ptr;
    }

    private static IntPtr RPCCbFunc(IntPtr bufferPtr)
    {
      var buffer = (MemoryBuffer) Marshal.PtrToStructure(bufferPtr, typeof(MemoryBuffer));
      Request req = new Request();
      req.MergeFrom(new CodedInputStream(buffer.GetData()));

      Response response;
      switch (req.Type)
      {
        case RPCType.User:
          response = HandleRpc(req, RPCType.User);
          break;
        case RPCType.Sys:
          response = HandleRpc(req, RPCType.Sys);
          break;
        default:
          throw new Exception($"invalid rpc type, argument:{req.Type}");
      }
      
      var res = new MemoryBuffer();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));

      byte[] responseBytes = ProtoMessageToByteArray(response);
      res.data = ByteArrayToIntPtr(responseBytes);
      res.size = responseBytes.Length;
      Marshal.StructureToPtr(res, pnt, false);
      
      return pnt;
    }

    internal static Response HandleRpc(Protos.Request req, RPCType type)
    {

      byte[] data = req.Msg.Data.ToByteArray();
      Route route = Route.FromString(req.Msg.Route);

      string handlerName = $"{route.service}.{route.method}";

      var s = new Models.Session(req.Session, req.FrontendID);
      var response = new Protos.Response();

      RemoteMethod handler;
      if (type == RPCType.Sys)
      {
          if (!HandlersDict.ContainsKey(handlerName))
          {
            response = GetErrorResponse("PIT-404", $"remote/handler not found! remote/handler name: {handlerName}");
            return response;
          }       
          handler = HandlersDict[handlerName];
      } else
      {
          if (!RemotesDict.ContainsKey(handlerName))
          {
            response = GetErrorResponse("PIT-404", $"remote/handler not found! remote/handler name: {handlerName}");
            return response;
          }       
          handler = RemotesDict[handlerName];
      }
      
      try
      {
        IMessage ans;
        if (handler.ArgType != null)
        {
            var arg = (IMessage) Activator.CreateInstance(handler.ArgType);
            arg.MergeFrom(new CodedInputStream(data));
            ans = (IMessage) handler.Method.Invoke(handler.Obj, new object[] {s, arg}); // TODO if only one this can crash
        }
        else
        {
            ans = (IMessage) handler.Method.Invoke(handler.Obj, new object[] {s});
        }
        byte[] ansBytes = ProtoMessageToByteArray(ans);
        response.Data = ByteString.CopyFrom(ansBytes);
        return response;
      }
      catch (Exception e)
      {
        if (e.InnerException != null)
          Logger.Error("Exception thrown in handler, error:{0}",
            e.InnerException.Message); // TODO externalize method and only print stacktrace when debug
        response = GetErrorResponse("PIT-500", e.Message);
        return response;
      }
    }

    private static T GetProtoMessageFromMemoryBuffer<T>(MemoryBuffer rpcRes)
    {
      byte[] resData = rpcRes.GetData();
      var response = new Protos.Response();
      response.MergeFrom(new CodedInputStream(resData));
      var res = (IMessage) Activator.CreateInstance(typeof(T));
      res.MergeFrom(response.Data);
      Logger.Debug("getProtoMsgFromResponse: got this res {0}", res);
      return (T) res;
    }

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
      string className = remoteMethod.GetName();
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
      string className = handlerMethod.GetName();
      RegisterHandler(handlerMethod, className, DefaultRemoteNameFunc);
    }

    public static void RegisterHandler(BaseHandlerMethod handlerMethod, string name)
    {
      RegisterHandler(handlerMethod, name, DefaultRemoteNameFunc);
    }

    public static void RegisterHandler(BaseHandlerMethod handlerMethod, string name, RemoteNameFunc remoteNameFunc) // TODO remote function name func
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

    public static unsafe Response SendPushToUser(string userId, string frontendId, string serverType, Push push)
    {
      bool ok = false;
      MemoryBuffer inMemBuf = new MemoryBuffer();
      MemoryBuffer* outMemBufPtr = null;
      var retError = new Error();

      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(inMemBuf));
      try
      {
        var data = ProtoMessageToByteArray(push);
        fixed (byte* p = data)
        {
          inMemBuf.data = (IntPtr) p;
          inMemBuf.size = data.Length;

          Marshal.StructureToPtr(inMemBuf, pnt, false);
          ok = PushInternal(userId, frontendId, serverType, pnt, &outMemBufPtr, ref retError);
          if (!ok) // error
          {
            throw new PitayaException($"Push failed: ({retError.code}: {retError.msg})"); //TODO exception or error?
          }

          Response response = new Response();
          response.MergeFrom(new CodedInputStream(outMemBufPtr->GetData()));
          return response;
        }
      }
      finally
      {
        Marshal.FreeHGlobal(pnt);
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

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_InitializeWithNats")]
    private static extern bool InitializeWithNatsInternal(IntPtr natsCfg, IntPtr sdCfg, IntPtr server, IntPtr cbPtr, IntPtr freePtr, NativeLogLevel logLevel, string logFile);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_InitializeWithGrpc")]
    private static extern bool InitializeWithGrpcInternal(IntPtr grpcCfg, IntPtr sdCfg, IntPtr server, IntPtr cbPtr, IntPtr freePtr, NativeLogLevel logLevel,  string logFile);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Terminate")]
    private static extern void TerminateInternal();

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_GetServerById")]
    private static extern bool GetServerByIdInternal(string serverId, ref Server retServer);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeServer")]
    private static extern void FreeServerInternal(IntPtr serverPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_RPC")]
    private static extern unsafe bool RPCInternal(string serverId, string route, IntPtr data, int dataSize, MemoryBuffer** buffer, ref Error retErr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeMemoryBuffer")]
    private static extern unsafe void FreeMemoryBufferInternal(MemoryBuffer *ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_OnSignal")]
    private static extern void OnSignalInternal(OnSignalFunc ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreePitayaError")]
    private static extern unsafe void FreePitayaErrorInternal(ref Error err);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_PollGRPC")]
    private static extern void PollGRPC();
    
    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_SendPushToUser")]
    private static extern unsafe bool PushInternal(string userId, string serverId, string serverType, IntPtr pushData, MemoryBuffer** buffer, ref Error retErr);
  }
}
