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
    public delegate IntPtr RPCCb(RPCReq req);
    public delegate string RemoteNameFunc(string methodName);

    private static Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    public static Protos.Response GetErrorResponse(string code, string msg) {
      var response = new Protos.Response();
      response.Error = new Protos.Error();
      response.Error.Code = code;
      response.Error.Msg = msg;
      return response;
    }

    public static byte[] ProtoMessageToByteArray(IMessage msg){
      var mem = new MemoryStream();
      var o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      return mem.ToArray();
    }

    public static Server GetServer(string id) {
      var pnt = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Server)));
      var ret = new Server();
      try
      {
        Marshal.StructureToPtr(ret, pnt, false);
        bool success = GetServerInternal(GoString.fromString(id), pnt);
        if (success){
          ret = (Server) Marshal.PtrToStructure(pnt, typeof(Server));
          FreeServer(pnt);
          return ret;
        }
        throw new Exception("failed to get server! with id " + id);
      } finally
      {
        Marshal.FreeHGlobal(pnt);
      }
    }

    public static void RegisterRemote(BaseRemote remote) {
      string className = remote.GetType().Name.ToLower();
      RegisterRemote(remote, className, Utils.DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name) {
      RegisterRemote(remote, name, Utils.DefaultRemoteNameFunc);
    }

    public static void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc) {
      Dictionary<string,  RemoteMethod> m = remote.getRemotesMap();
      foreach (KeyValuePair<string, RemoteMethod> kvp in m){
        var rn = remoteNameFunc(kvp.Key);
        var remoteName = String.Format("{0}.{1}", name, rn);
        if (remotesDict.ContainsKey(remoteName)){
          throw new Exception(String.Format("tried to register same remote twice! remote name: {0}",remoteName));
        }
        Logger.Info("registering remote {0}", remoteName);
        remotesDict[remoteName] = kvp.Value;
      }
    }

    // TODO can we make this faster with some delegate-fu?
    private static IntPtr RPCCbFunc(RPCReq req) {
      byte[] data = req.getReqData();
      Route route = Route.fromString(req.route);
      Logger.Debug("called with route: " + route + " and data: " + Encoding.UTF8.GetString(data));
      var response = new Protos.Response();
      string remoteName = String.Format("{0}.{1}", route.service, route.method);
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(new GoSlice()));
      if (!remotesDict.ContainsKey(remoteName)){
        response = GetErrorResponse("PIT-404", String.Format("remote not found! remote name: {0}", remoteName));
        byte[] resBytes = ProtoMessageToByteArray(response);
        Marshal.StructureToPtr(GoSlice.fromSlice<byte>(resBytes), pnt, false);
        return pnt;
      }
      RemoteMethod remote = remotesDict[remoteName];
      Logger.Debug(String.Format("found delegate: {0}", remote));
      try{
        var arg = (IMessage) Activator.CreateInstance(remote.argType);
        arg.MergeFrom(new CodedInputStream(data));
        // invoke is slow :/
        var ans = (IMessage) remote.method.Invoke(remote.obj, new object[]{arg});
        byte[] ansBytes = ProtoMessageToByteArray(ans);
        response.Data = ByteString.CopyFrom(ansBytes);
        byte[] resBytes = ProtoMessageToByteArray(response);
        Marshal.StructureToPtr(GoSlice.fromSlice<byte>(resBytes), pnt, false);
        return pnt;
      } catch(Exception e) {
        response = GetErrorResponse("PIT-500", e.Message);
        byte[] resBytes = ProtoMessageToByteArray(response);
        Marshal.StructureToPtr(GoSlice.fromSlice<byte>(resBytes), pnt, false);
        return pnt;
      }
    }

    private static T GetProtoMessageFromResponse<T>(RPCRes rpcRes) {
      byte[] resData = rpcRes.getResData();
      var response = new Protos.Response();
      response.MergeFrom(new CodedInputStream(resData));
      var res = (IMessage) Activator.CreateInstance(typeof(T));
      res.MergeFrom(new CodedInputStream(response.Data.ToByteArray()));
      Logger.Debug("getProtoMsgFromResponse: got this res {0}", res);

      return (T)res;
    }

    public static T RPC<T>(string serverId, Route route, IMessage msg) {
      byte[] data = ProtoMessageToByteArray(msg);
      var ret = new RPCRes();
      IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(RPCRes)));
      try
      {
        Marshal.StructureToPtr(ret, pnt, false);
        bool success = SendRPC(GoString.fromString(serverId), route, GoSlice.fromSlice<byte>(data), pnt);
        if (success){
          ret = (RPCRes) Marshal.PtrToStructure(pnt, typeof(RPCRes));
          T protoRet = GetProtoMessageFromResponse<T>(ret); 
          FreeRPCRes(pnt);
          return protoRet;
        } 
        throw new Exception("RPC call failed!");
      } finally
      {
        Marshal.FreeHGlobal(pnt);
      }
    }

    public static T RPC<T>(Route route, IMessage msg) {
      return RPC<T>("", route, msg);
    }

    public static void InitDefault(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server) {
        if (!InitServerInternal(server) || !SetSDEtcdInternal(sdConfig) || !SetRPCNatsInternal(rpcClientConfig, rpcServerConfig) || !StartInternal()) {
            throw new Exception("failed to initialize pitaya go module");
        }
    }

    public static void Init(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server) {
      Logger.Info("initializing pitaya module");
      InitDefault(sdConfig, rpcClientConfig, rpcServerConfig, server);
      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate (rpcCbFunc);
      // hack, inject pointer to cb function into Go code
      PitayaCluster.SetRPCCallbackInternal(rpcCbFuncPtr);
      Logger.Info("initialized pitaya go module");
    }

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      public static extern bool Shutdown();

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      public static extern bool ConfigureJaeger(double probability, GoString serviceName);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "InitServer")]
      static extern bool InitServerInternal(Server server);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetSDEtcd")]
      static extern bool SetSDEtcdInternal(SDConfig sdConfig);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetRPCNats")]
      static extern bool SetRPCNatsInternal(NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "Start")]
      static extern bool StartInternal();

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
      static extern bool GetServerInternal(GoString id, IntPtr ret);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern bool SendRPC(GoString svId, Route route, GoSlice message, IntPtr ret);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      public static extern void FreeServer(IntPtr ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern void FreeRPCRes(IntPtr ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetRPCCallback")]
      static extern void SetRPCCallbackInternal(IntPtr funcPtr);
  }
}
