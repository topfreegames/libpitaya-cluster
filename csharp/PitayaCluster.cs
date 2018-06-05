using System;
using System.Runtime.InteropServices;
using System.Text;


namespace Pitaya
{
  class PitayaCluster
  {
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

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      public static extern int Init(SDConfig sdConfig, Server server);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
      static extern PtrResWithStatus GetServerInternal(GoString id);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServersByType")]
      static extern PtrResWithStatus GetServersByTypeInternal(GoString type);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern void FreeServer(IntPtr ptr);

  }
}
