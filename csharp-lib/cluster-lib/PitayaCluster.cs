using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Google.Protobuf;
using System.Collections.Generic;

namespace Pitaya
{
  public class PitayaCluster
  {
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate IntPtr RPCCb(IntPtr req);
    public delegate string RemoteNameFunc(string methodName);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void FreeHGlobalDelegate(IntPtr ptr);

    public delegate void OnSignalFunc();

    public delegate void LogHandler(string msg);

    public delegate void SignalHandler();

    private static Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    private static Action onSignalEvent;

    private static Protos.Response GetErrorResponse(string code, string msg)
    {
      var response = new Protos.Response {Error = new Protos.Error {Code = code, Msg = msg}};
      return response;
    }

    private static byte[] ProtoMessageToByteArray(IMessage msg)
    {
      if (msg == null)
      {
        return new byte[]{};
      }

      var mem = new MemoryStream();
      var o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      return mem.ToArray();
    }

    public static void RegisterRemote(BaseRemote remote)
    {
      string className = remote.GetType().Name.ToLower();
      RegisterRemote(remote, className, Utils.DefaultRemoteNameFunc);
    }

    public void RegisterRemote(BaseRemote remote, string name)
    {
      RegisterRemote(remote, name, Utils.DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc)
    {
      Dictionary<string, RemoteMethod> m = remote.getRemotesMap();
      foreach (KeyValuePair<string, RemoteMethod> kvp in m)
      {
        var rn = remoteNameFunc(kvp.Key);
        var remoteName = $"{name}.{rn}";
        if (remotesDict.ContainsKey(remoteName))
        {
          throw new PitayaException($"tried to register same remote twice! remote name: {remoteName}");
        }
        Logger.Info("registering remote {0}", remoteName);
        remotesDict[remoteName] = kvp.Value;
      }
    }

    private static IntPtr ByteArrayToIntPtr(byte[] data)
    {
      IntPtr ptr = Marshal.AllocHGlobal(data.Length);
      Marshal.Copy(data, 0, ptr, data.Length);
      return ptr;
    }

    // TODO can we make this faster with some delegate-fu?
    private static IntPtr RPCCbFunc(IntPtr reqPtr)
    {
      var req = (RPCReq)Marshal.PtrToStructure(reqPtr, typeof(RPCReq));
      byte[] data = req.GetReqData();
      Route route = Route.FromString(req.route);
      Logger.Debug($"called with route: {route} and data: {Encoding.UTF8.GetString(data)}");
      string remoteName = $"{route.service}.{route.method}";
      var response = new Protos.Response();

      var res = new MemoryBuffer();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));

      if (!remotesDict.ContainsKey(remoteName))
      {
        response = GetErrorResponse("PIT-404", $"remote not found! remote name: {remoteName}");
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }

      RemoteMethod remote = remotesDict[remoteName];
      Logger.Debug($"found delegate: {remote}");

      try
      {
        var arg = (IMessage)Activator.CreateInstance(remote.argType);
        arg.MergeFrom(new CodedInputStream(data));
        // invoke is slow :/
        var ans = (IMessage)remote.method.Invoke(remote.obj, new object[] { arg });
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
      var res = (IMessage)Activator.CreateInstance(typeof(T));
      res.MergeFrom(response.Data);
      Logger.Debug("getProtoMsgFromResponse: got this res {0}", res);
      return (T)res;
    }

    public static void AddSignalHandler(Action cb)
    {
      onSignalEvent += cb;
      OnSignalInternal(OnSignal);
    }

    private static void OnSignal()
    {
      Logger.Info("Invoking signal handler");
      onSignalEvent?.Invoke();
    }

    public static void Initialize(GrpcConfig grpcCfg, SDConfig sdCfg, Server server, NativeLogLevel logLevel, string logFile = "")
    {
      IntPtr grpcCfgPtr = new StructWrapper(grpcCfg);
      IntPtr sdCfgPtr = new StructWrapper(sdCfg);
      IntPtr serverPtr = new StructWrapper(server);

      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);

      FreeHGlobalDelegate freeDelegate = Marshal.FreeHGlobal;
      IntPtr freeHGlobalPtr = Marshal.GetFunctionPointerForDelegate(freeDelegate);

      bool ok = InitializeWithGrpcInternal(grpcCfgPtr, sdCfgPtr, serverPtr, rpcCbFuncPtr, freeHGlobalPtr, logLevel, logFile);

      if (!ok)
      {
        throw new PitayaException("Initialization failed");
      }
    }

    public static void Initialize(NatsConfig natsCfg, SDConfig sdCfg, Server server, NativeLogLevel logLevel, string logFile = "")
    {
      IntPtr natsCfgPtr = new StructWrapper(natsCfg);
      IntPtr sdCfgPtr = new StructWrapper(sdCfg);
      IntPtr serverPtr = new StructWrapper(server);

      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);

      FreeHGlobalDelegate freeDelegate = Marshal.FreeHGlobal;
      IntPtr freeHGlobalPtr = Marshal.GetFunctionPointerForDelegate(freeDelegate);

      bool ok = InitializeWithNatsInternal(natsCfgPtr, sdCfgPtr, serverPtr, rpcCbFuncPtr, freeHGlobalPtr, logLevel, logFile);

      if (!ok)
      {
        throw new PitayaException("Initialization failed");
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
  }
}
