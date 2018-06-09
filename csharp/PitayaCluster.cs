using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Google.Protobuf;
using System.Collections.Generic;
using System.Reflection;

namespace Pitaya
{
  // TODO debug logging and levels
  class PitayaCluster
  {
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate GoSlice RPCCb(RPCReq req);

    // TODO criar algo ja com tipo para mais ferformance
    private static Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    public static Protos.Response getErrorResponse(string code, string msg) {
      Protos.Response response = new Protos.Response();
      response.Error = new Protos.Error();
      response.Error.Code = code;
      response.Error.Msg = msg;
      return response;
    }

    public static byte[] protoMessageToByteArray(IMessage msg){
       MemoryStream mem = new MemoryStream();
      CodedOutputStream o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      return mem.ToArray();
    }

    public static Server GetServer(string id) {
      PtrResWithStatus res = GetServerInternal(GoString.fromString(id));
      if (res.status == 0){
        Server sv = res.getStructFromPtr<Server>();
        FreeServer(res.ptr); // free server in golang side because of allocated memory (prevent memory leak)
        return sv;
      } else {
        throw new Exception("failed to get server! with id " + id);
      }
    }

    public static Server[] GetServers(string type) {
      PtrResWithStatus res = GetServersByTypeInternal(GoString.fromString(type));
      if (res.status == 0){
        GoSlice slice = res.getStructFromPtr<GoSlice>();
        Server[] servers = slice.toSlice<Server>(true);
        // free all servers
        IntPtr addr = slice.data;
        for (int i = 0; i < slice.len; i++){
          IntPtr ptr = (IntPtr)Marshal.PtrToStructure(addr, typeof(IntPtr));
          FreeServer(ptr);
          addr += Marshal.SizeOf(typeof(IntPtr));
        }
        return servers;
      } else {
        throw new Exception("failed to get servers with type " + type);
      }
    }

    private static T GetProtoMessageFromResponse<T>(RPCRes rpcRes) {
      byte[] resData = rpcRes.getResData();
      Protos.Response response = new Protos.Response();
      response.MergeFrom(new CodedInputStream(resData));
      IMessage res = (IMessage) Activator.CreateInstance(typeof(T));
      res.MergeFrom(new CodedInputStream(response.Data.ToByteArray()));
      Console.WriteLine("got this res {0}", res);
      return (T)res;
    }

    public static void RegisterRemote(BaseRemote remote) {
      string className = remote.GetType().Name.ToLower();
      Dictionary<string,  RemoteMethod> m = remote.getRemotesMap();
      foreach (KeyValuePair<string, RemoteMethod> kvp in m){
        string remoteName = String.Format("{0}.{1}", className, kvp.Key);
        if (remotesDict.ContainsKey(remoteName)){
          throw new Exception(String.Format("tried to register same remote twice! remote name: {0}",remoteName));
        }
        Console.WriteLine("registering remote {0}", remoteName);
        remotesDict[remoteName] = kvp.Value;
      }
    }

    // TODO can we make this faster with some delegate-fu?
    private static GoSlice RPCCbFunc(RPCReq req) {
      byte[] data = req.getReqData();
      Route route = Route.fromString(req.route);
      Console.WriteLine("called with route: " + route + " and data: " + Encoding.UTF8.GetString(data));
      Protos.Response response = new Protos.Response();
      string remoteName = String.Format("{0}.{1}", route.service, route.method);
      if (!remotesDict.ContainsKey(remoteName)){
        response = getErrorResponse("PIT-404", String.Format("remote not found! remote name: {0}", remoteName));
        byte[] resBytes = protoMessageToByteArray(response);
        return GoSlice.fromSlice<byte>(resBytes);
      }
      RemoteMethod remote = remotesDict[remoteName];
      Console.WriteLine(String.Format("found delegate: {0}", remote));
      try{
        IMessage arg = (IMessage) Activator.CreateInstance(remote.argType);
        arg.MergeFrom(new CodedInputStream(data));
        // invoke is slow :/
        IMessage ans = (IMessage) remote.method.Invoke(remote.obj, new object[]{arg});
        byte[] ansBytes = protoMessageToByteArray(ans);
        response.Data = ByteString.CopyFrom(ansBytes);
        byte[] resBytes = protoMessageToByteArray(response);
        return GoSlice.fromSlice<byte>(resBytes);
      } catch(Exception e) {
        response = getErrorResponse("PIT-500", e.Message);
        byte[] resBytes = protoMessageToByteArray(response);
        return GoSlice.fromSlice<byte>(resBytes);
      }
    }

    public static T RPC<T>(Server server, Route route, IMessage msg) {
      byte[] data = protoMessageToByteArray(msg);
      IntPtr ptr = SendRPC(GoString.fromString(server.id), route, GoSlice.fromSlice<byte>(data));
      RPCRes ret = (RPCRes)Marshal.PtrToStructure(ptr, typeof(RPCRes));
      if (ret.success){
        return GetProtoMessageFromResponse<T>(ret);
      } else {
        throw new Exception("RPC call failed!");
      }
    }

    public static T RPC<T>(Route route, IMessage msg) {
      byte[] data = protoMessageToByteArray(msg);
      IntPtr ptr = SendRPC(GoString.fromString(""), route, GoSlice.fromSlice<byte>(data));
      RPCRes ret = (RPCRes)Marshal.PtrToStructure(ptr, typeof(RPCRes));
      if (ret.success){
        return GetProtoMessageFromResponse<T>(ret);
      } else {
        throw new Exception("RPC call failed!");
      }
    }

    public static void Init(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server) {
      int res = InitInternal(sdConfig, rpcClientConfig, rpcServerConfig, server);
      if(res == 0){
        Console.WriteLine("initialized pitaya go module");
        PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
        IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate (rpcCbFunc);
        // hack, inject pointer to cb function into Go code
        PitayaCluster.SetRPCCallbackInternal(rpcCbFuncPtr);
      } else {
        throw new Exception("failed to initialize pitaya go module");
      }
    }

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "Init")]
      static extern int InitInternal(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
      static extern PtrResWithStatus GetServerInternal(GoString id);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServersByType")]
      static extern PtrResWithStatus GetServersByTypeInternal(GoString type);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern IntPtr SendRPC(GoString svId, Route route, GoSlice message);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern void FreeServer(IntPtr ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetRPCCallback")]
      static extern void SetRPCCallbackInternal(IntPtr funcPtr);
  }
}
