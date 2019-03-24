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

    public delegate void LogHandler(string msg);

    public delegate void SignalHandler();

    private static Dictionary<string, RemoteMethod> _remotesDict = new Dictionary<string, RemoteMethod>();
    private static Dictionary<string, HandlerMethod> _handlersDict = new Dictionary<string, HandlerMethod>();

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

    // TODO can we make this faster with some delegate-fu?
    private static IntPtr RPCCbFunc(IntPtr bufferPtr)
    {
      // TODO break this awful method
      var buffer = (MemoryBuffer) Marshal.PtrToStructure(bufferPtr, typeof(MemoryBuffer));
      Request req = new Request();
      req.MergeFrom(new CodedInputStream(buffer.GetData()));

      //Logger.Debug($"called with type: {req.Type} route: {req.Msg.Route}");

      switch (req.Type)
      {
        case RPCType.User:
          return HandleRPCUser(req);
        case RPCType.Sys:
          return HandleRPCSys(req);
        default:
          throw new Exception($"invalid rpc type, argument:{req.Type}");
      }
    }

    private static IntPtr HandleRPCSys(Protos.Request req)
    {

      byte[] data = req.Msg.Data.ToByteArray();
      Route route = Route.FromString(req.Msg.Route);

      string handlerName = $"{route.service}.{route.method}";

      var s = new Models.Session(req.Session, req.FrontendID);
      var response = new Protos.Response();

      var res = new MemoryBuffer();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));
      if (!_handlersDict.ContainsKey(handlerName))
      {
        response = GetErrorResponse("PIT-404", $"handler not found! handler name: {handlerName}");
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }

      HandlerMethod handler = _handlersDict[handlerName];
      //Logger.Debug($"found delegate: {handler}");

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
        // invoke is slow :/
        byte[] ansBytes = ProtoMessageToByteArray(ans);
        response.Data = ByteString.CopyFrom(ansBytes);
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }
      catch (Exception e)
      {
        if (e.InnerException != null)
          Logger.Error("Exception thrown in handler, error:{0}",
            e.InnerException.Message); // TODO externalize method and only print stacktrace when debug
        response = GetErrorResponse("PIT-500", e.Message);
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }
    }

    private static IntPtr HandleRPCUser(Protos.Request req)
    {
      byte[] data = req.Msg.Data.ToByteArray();
      Route route = Route.FromString(req.Msg.Route);

      string remoteName = $"{route.service}.{route.method}";
      var response = new Protos.Response();

      var res = new MemoryBuffer();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));

      if (!_remotesDict.ContainsKey(remoteName))
      {
        response = GetErrorResponse("PIT-404", $"remote not found! remote name: {remoteName}");
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }

      RemoteMethod remote = _remotesDict[remoteName];
      Logger.Debug($"found delegate: {remote}");

      try
      {
        var arg = (IMessage) Activator.CreateInstance(remote.argType);
        arg.MergeFrom(new CodedInputStream(data));
        // invoke is slow :/
        var ans = (IMessage) remote.method.Invoke(remote.obj, new object[] {arg});
        byte[] ansBytes = ProtoMessageToByteArray(ans);
        response.Data = ByteString.CopyFrom(ansBytes);
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }
      catch (Exception e)
      {
        if (e.InnerException != null)
          Logger.Error("Exception thrown in handler, error:{0}",
            e.InnerException.Message);
        response = GetErrorResponse("PIT-500", e.Message);
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
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

    public static void ListenGRPC()
    {
      PollGRPC();
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

    public static void RegisterRemote(BaseRemote remote)
    {
      string className = remote.GetName();
      RegisterRemote(remote, className, DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name)
    {
      RegisterRemote(remote, name, DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc) // TODO remote function name func
    {
      Dictionary<string, RemoteMethod> m = remote.getRemotesMap();
      foreach (KeyValuePair<string, RemoteMethod> kvp in m)
      {
        var rn = remoteNameFunc(kvp.Key);
        var remoteName = $"{name}.{rn}";
        if (_remotesDict.ContainsKey(remoteName))
        {
          throw new PitayaException($"tried to register same remote twice! remote name: {remoteName}");
        }
        Logger.Info("registering remote {0}", remoteName);
        _remotesDict[remoteName] = kvp.Value;
      }
    }
    
    public static void RegisterHandler(BaseHandler handler)
    {
      string className = handler.GetName();
      RegisterHandler(handler, className, DefaultRemoteNameFunc);
    }

    public static void RegisterHandler(BaseHandler handler, string name)
    {
      RegisterHandler(handler, name, DefaultRemoteNameFunc);
    }

    public static void RegisterHandler(BaseHandler handler, string name, RemoteNameFunc remoteNameFunc) // TODO remote function name func
    {
      Dictionary<string, HandlerMethod> m = handler.getHandlerMap();
      foreach (KeyValuePair<string, HandlerMethod> kvp in m)
      {
        var rn = remoteNameFunc(kvp.Key);
        var handlerName = $"{name}.{rn}";
        if (_handlersDict.ContainsKey(handlerName))
        {
          throw new PitayaException($"tried to register same remote twice! remote name: {handlerName}");
        }
        Logger.Info("registering handler {0}", handlerName);
        _handlersDict[handlerName] = kvp.Value;
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

    public static unsafe T Rpc<T>(string serverId, Route route, IMessage msg)
    {
      bool ok = false;
      MemoryBuffer* memBufPtr = null;
      var retError = new Error();

      var data = ProtoMessageToByteArray(msg);
      fixed (byte* p = data)
      {
        ok = RPCInternal(serverId, route.ToString(), (IntPtr)p, data.Length, &memBufPtr, ref retError);
      }

      if (!ok) // error
      {
        throw new PitayaException($"RPC call failed: ({retError.code}: {retError.msg})");
      }

      var protoRet = GetProtoMessageFromMemoryBuffer<T>(*memBufPtr);
      FreeMemoryBufferInternal(memBufPtr);
      return protoRet;
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
  }
}
