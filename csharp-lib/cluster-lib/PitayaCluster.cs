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

    public delegate void OnSignalFunc();

    private static Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    public static Protos.Response GetErrorResponse(string code, string msg)
    {
      var response = new Protos.Response();
      response.Error = new Protos.Error();
      response.Error.Code = code;
      response.Error.Msg = msg;
      return response;
    }

    public static byte[] ProtoMessageToByteArray(IMessage msg)
    {
      var mem = new MemoryStream();
      var o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      return mem.ToArray();
    }
    //
    //    public static Server GetServer(string id)
    //    {
    //      var pnt = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Server)));
    //      var ret = new Server();
    //      try
    //      {
    //        Marshal.StructureToPtr(ret, pnt, false);
    //        bool success = GetServerInternal(GoString.fromString(id), pnt);
    //        if (success)
    //        {
    //          ret = (Server)Marshal.PtrToStructure(pnt, typeof(Server));
    //          FreeServer(pnt);
    //          return ret;
    //        }
    //        throw new Exception("failed to get server! with id " + id);
    //      }
    //      finally
    //      {
    //        Marshal.FreeHGlobal(pnt);
    //      }
    //    }
    //
    public static void RegisterRemote(BaseRemote remote)
    {
      string className = remote.GetType().Name.ToLower();
      RegisterRemote(remote, className, Utils.DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name)
    {
      RegisterRemote(remote, name, Utils.DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc)
    {
      Dictionary<string, RemoteMethod> m = remote.getRemotesMap();
      foreach (KeyValuePair<string, RemoteMethod> kvp in m)
      {
        var rn = remoteNameFunc(kvp.Key);
        var remoteName = String.Format("{0}.{1}", name, rn);
        if (remotesDict.ContainsKey(remoteName))
        {
          throw new Exception(String.Format("tried to register same remote twice! remote name: {0}", remoteName));
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
      Route route = Route.fromString(req.route);
      Logger.Debug("called with route: " + route.ToString() + " and data: " + Encoding.UTF8.GetString(data));
      string remoteName = String.Format("{0}.{1}", route.service, route.method);
      var response = new Protos.Response();

      var res = new MemoryBuffer();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));
      if (!remotesDict.ContainsKey(remoteName))
      {
        response = GetErrorResponse("PIT-404", String.Format("remote not found! remote name: {0}", remoteName));
        byte[] responseBytes = ProtoMessageToByteArray(response);
        res.data = ByteArrayToIntPtr(responseBytes);
        res.size = responseBytes.Length;
        Marshal.StructureToPtr(res, pnt, false);
        return pnt;
      }
      RemoteMethod remote = remotesDict[remoteName];
      Logger.Debug(String.Format("found delegate: {0}", remote));
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
      res.MergeFrom(new CodedInputStream(response.Data.ToByteArray()));
      Logger.Debug("getProtoMsgFromResponse: got this res {0}", res);
      return (T)res;
    }
    //
    //    public static T RPC<T>(Route route, IMessage msg)
    //    {
    //      return RPC<T>("", route, msg);
    //    }
    //
    //    public static void InitDefault(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server)
    //    {
    //      if (!InitServerInternal(server) || !SetSDEtcdInternal(sdConfig) || !SetRPCNatsInternal(rpcClientConfig, rpcServerConfig) || !StartInternal())
    //      {
    //        throw new Exception("failed to initialize pitaya go module");
    //      }
    //    }
    //
    //    public static void InitGrpc(SDConfig sdConfig, GrpcRPCClientConfig rpcClientConfig, GrpcRPCServerConfig rpcServerConfig, Server server)
    //    {
    //      if (!InitServerInternal(server) || !SetSDEtcdInternal(sdConfig) || !SetRPCGrpcInternal(rpcClientConfig, rpcServerConfig) || !StartInternal())
    //      {
    //        throw new Exception("failed to initialize pitaya go module");
    //      }
    //    }
    //
    //    public static void Init(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server)
    //    {
    //      Logger.Info("initializing pitaya module with nats");
    //      InitDefault(sdConfig, rpcClientConfig, rpcServerConfig, server);
    //      SetRPCCbs();
    //    }
    //
    //    public static void Init(SDConfig sdConfig, GrpcRPCClientConfig rpcClientConfig, GrpcRPCServerConfig rpcServerConfig, Server server)
    //    {
    //      Logger.Info("initializing pitaya module with grpc");
    //      InitGrpc(sdConfig, rpcClientConfig, rpcServerConfig, server);
    //      SetRPCCbs();
    //    }
    //
    //    public static void SetRPCCbs()
    //    {
    //      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
    //      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);
    //      // hack, inject pointer to cb function into Go code
    //      PitayaCluster.SetRPCCallbackInternal(rpcCbFuncPtr);
    //      Logger.Info("initialized pitaya go module");
    //    }
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
    //    public static extern bool Shutdown();
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
    //    public static extern bool ConfigureJaeger(double probability, GoString serviceName);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "InitServer")]
    //    static extern bool InitServerInternal(Server server);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetSDEtcd")]
    //    static extern bool SetSDEtcdInternal(SDConfig sdConfig);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetRPCNats")]
    //    static extern bool SetRPCNatsInternal(NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetRPCGrpc")]
    //    static extern bool SetRPCGrpcInternal(GrpcRPCClientConfig rpcClientConfig, GrpcRPCServerConfig rpcServerConfig);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "Start")]
    //    static extern bool StartInternal();
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "GetServer")]
    //    static extern bool GetServerInternal(GoString id, IntPtr ret);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
    //    static extern bool SendRPC(GoString svId, Route route, GoSlice message, IntPtr ret);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
    //    public static extern void FreeServer(IntPtr ptr);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
    //    static extern void FreeRPCRes(IntPtr ptr);
    //
    //    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetRPCCallback")]
    //    static extern void SetRPCCallbackInternal(IntPtr funcPtr);

    private static void OnSignal()
    {
      Logger.Info("================================================");
      Logger.Info("Recevied SIGNAL FROM C++ :D");
      Logger.Info("================================================");
      Shutdown();
      System.Environment.Exit(0);
    }

    public static bool InitDefault(SDConfig sdConfig, NatsConfig natsCfg, Server server)
    {
      IntPtr sdCfgPtr = new StructWrapper(sdConfig);
      IntPtr natsCfgPtr = new StructWrapper(natsCfg);
      IntPtr serverPtr = new StructWrapper(server);

      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);
      OnSignalInternal(OnSignal);
      return InitializeInternal(serverPtr, natsCfgPtr, rpcCbFuncPtr);
    }

    public static bool Init(SDConfig sdConfig, NatsConfig natsCfg, Server server)
    {
      return InitDefault(sdConfig, natsCfg, server);
    }

    public static void Shutdown()
    {
      ShutdownInternal();
    }

    public static Nullable<Pitaya.Server> GetServerById(string serverId)
    {
      IntPtr serverPtr = GetServerByIdInternal(serverId);

      if (serverPtr == IntPtr.Zero)
      {
        Logger.Error("There are no servers with id " + serverId);
        return null;
      }

      var server = (Pitaya.Server)Marshal.PtrToStructure(serverPtr, typeof(Pitaya.Server));
      FreeServerInternal(serverPtr);
      return server;
    }

    public static T RPC<T>(string serverId, Route route, IMessage msg)
    {
      byte[] data = ProtoMessageToByteArray(msg);
      var memBuf = new MemoryBuffer();
      IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(MemoryBuffer)));

      try
      {
        Marshal.StructureToPtr(memBuf, ptr, false);
        IntPtr pitayaErr = RPCInternal(serverId, route.ToString(), data, data.Length, out ptr);

        if (pitayaErr != IntPtr.Zero) // error
        {
          var err = (Pitaya.Error)Marshal.PtrToStructure(pitayaErr, typeof(Pitaya.Error));
          FreePitayaErrorInternal(pitayaErr);
          throw new Exception(string.Format("RPC call failed: ({0}: {1})", err.code, err.msg));
        }

        memBuf = (MemoryBuffer)Marshal.PtrToStructure(ptr, typeof(MemoryBuffer));
        T protoRet = GetProtoMessageFromMemoryBuffer<T>(memBuf);
        FreeMemoryBufferInternal(ptr);
        return protoRet;
      }
      finally
      {
        Marshal.FreeHGlobal(ptr);
      }
    }

    public static T RPC<T>(Route route, IMessage msg)
    {
      return RPC<T>("", route, msg);
    }
    

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Initialize")]
    static extern bool InitializeInternal(IntPtr server, IntPtr natsCfg, IntPtr cbPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Shutdown")]
    static extern void ShutdownInternal();

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_GetServerById")]
    static extern IntPtr GetServerByIdInternal(string serverId);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeServer")]
    static extern void FreeServerInternal(IntPtr serverPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_RPC")]
    static extern IntPtr RPCInternal(string serverId, string route, byte[] data, int dataSize, out IntPtr buffer);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeMemoryBuffer")]
    static extern void FreeMemoryBufferInternal(IntPtr ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_OnSignal")]
    static extern void OnSignalInternal(OnSignalFunc ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreePitayaError")]
    static extern void FreePitayaErrorInternal(IntPtr ptr);
  }
}
