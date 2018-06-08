using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Google.Protobuf;


namespace Pitaya
{
  // TODO debug logging and levels
  class PitayaCluster
  {
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
      public delegate void RPCCb(RPCReq req);

    // return right
    private static void RPCCbWrapper(RPCReq req) {
      byte[] data = new byte[req.dataLen];
      Marshal.Copy(req.data, data, 0, req.dataLen);
      Console.WriteLine("called with route: " + req.route + " reply: " + req.replyTopic + " and data: " + Encoding.UTF8.GetString(data));
    }

    public static Server GetServer(string id) {
      PtrResWithStatus res = GetServerInternal(GoString.fromString(id));
      if (res.status == 0){
        Server sv = (Server) Marshal.PtrToStructure(res.ptr, typeof(Server));
        FreeServer(res.ptr); // free server in golang side because of allocated memory (prevent memory leak)
        return sv;
      } else {
        Console.WriteLine("failed to get server! with id " + id);
        return new Server();
      }
    }

    public static Server[] GetServers(string type) {
      PtrResWithStatus res = GetServersByTypeInternal(GoString.fromString(type));
      if (res.status == 0){
        GoSlice slice = (GoSlice) Marshal.PtrToStructure(res.ptr, typeof(GoSlice));
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
        Console.WriteLine("failed to get servers with type " + type);
        return new Server[0];
      }
    }

    // TODO get result
    // TODO deduplicate code with the below
    public static void RPC(Server server, Route route, IMessage msg) {
      MemoryStream mem = new MemoryStream();
      CodedOutputStream o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      PtrResWithStatus ret = SendRPC(GoString.fromString(server.id), route, GoSlice.fromSlice<byte>(mem.ToArray()));
      Console.WriteLine("received back " + ret);
    }

    public static void RPC(Route route, IMessage msg) {
      MemoryStream mem = new MemoryStream();
      CodedOutputStream o = new CodedOutputStream(mem);
      msg.WriteTo(o);
      o.Flush();
      mem.Close();
      PtrResWithStatus ret = SendRPC(GoString.fromString(""), route, GoSlice.fromSlice<byte>(mem.ToArray()));
      Console.WriteLine("received back " + ret);
    }

    // TODO return the init res
    public static void Init(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server) {
      int res = InitInternal(sdConfig, rpcClientConfig, rpcServerConfig, server);
      if(res == 0){
        Console.WriteLine("initialized pitaya go module");
        PitayaCluster.RPCCb rpcCbFunc = RPCCbWrapper;
        IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate (rpcCbFunc);
        // hack, inject pointer to cb function into Go code
        PitayaCluster.SetRPCCallbackInternal(rpcCbFuncPtr);
      } else {
        Console.WriteLine("failed to initialize pitaya go module");
      }
    }

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "Init")]
      static extern int InitInternal(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
      static extern PtrResWithStatus GetServerInternal(GoString id);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServersByType")]
      static extern PtrResWithStatus GetServersByTypeInternal(GoString type);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern PtrResWithStatus SendRPC(GoString svId, Route route, GoSlice message);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern void FreeServer(IntPtr ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetRPCCallback")]
      static extern void SetRPCCallbackInternal(IntPtr funcPtr);
  }
}
