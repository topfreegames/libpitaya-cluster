using System;
using System.Runtime.InteropServices;
using System.Text;


namespace myApp
{
    class Program
    {

      public struct GoSlice
      {
        public IntPtr data;
        public long len;
        public long cap;

        public GoSlice(IntPtr data, int len, int cap)
        {
          this.data = data;
          this.len = len;
          this.cap = cap;
        }

        public static GoSlice fromSlice<T>(T[] arr){
          GCHandle handle = GCHandle.Alloc(arr, GCHandleType.Pinned);
          try
          {
            IntPtr pointer = handle.AddrOfPinnedObject();
            GoSlice slice = new GoSlice(pointer, arr.Length, arr.Length);
            return slice;
          } finally
          {
            if (handle.IsAllocated)
            {
              handle.Free();
            }
          }
        }

        public T[] toSlice<T>(bool pointersInside) {
          T[] res = new T[this.len];
          IntPtr addr = this.data;
          for (int i = 0; i < this.len; i++){
            IntPtr ptr = addr;
            if (pointersInside) {
              ptr = (IntPtr)Marshal.PtrToStructure(addr, typeof(IntPtr));
            }
            T managedT = (T)Marshal.PtrToStructure(ptr, typeof(T));
            res[i] = managedT;
            addr += Marshal.SizeOf(typeof(IntPtr));
          }
          return res;
        }
      }

      public struct GoString
      {
        public IntPtr data;
        public long n;

        public GoString(IntPtr data, int n) {
          this.data = data;
          this.n = n;
        }

        public static GoString fromString(string str){
          byte[] bytes = Encoding.ASCII.GetBytes(str);
          GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
          try
          {
            IntPtr ptr = handle.AddrOfPinnedObject();
            GoString gstr = new GoString(ptr, str.Length);
            return gstr;
          } finally
          {
            if (handle.IsAllocated)
            {
              handle.Free();
            }
          }
        }

        public string toString() {
         return Marshal.PtrToStringAnsi(this.data, (int)this.n);
        }
      }

      public struct Server {
        [MarshalAs(UnmanagedType.LPStr)]
        public string id;

        [MarshalAs(UnmanagedType.LPStr)]
        public string type;

        [MarshalAs(UnmanagedType.LPStr)]
        public string metadata;
        public bool frontend;

        public Server(string id, string type, string metadata, bool frontend) {
          this.id = id;
          this.type = type;
          this.metadata = metadata;
          this.frontend = frontend;
        }
      }

      public struct PtrResWithStatus {
        public IntPtr ptr;
        public int status;

        public PtrResWithStatus(IntPtr ptr, int status) {
          this.ptr = ptr;
          this.status = status;
        }
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

      public static Server GetServers(string type) {
        PtrResWithStatus res = GetServersByTypeInternal(GoString.fromString(type));
        if (res.status == 0){
          GoSlice slice = (GoSlice) Marshal.PtrToStructure(res.ptr, typeof(GoSlice));
          Server[] serverPtrs = slice.toSlice<Server>(true);
         // free all servers
          IntPtr addr = slice.data;
          for (int i = 0; i < slice.len; i++){
            IntPtr ptr = (IntPtr)Marshal.PtrToStructure(addr, typeof(IntPtr));
            FreeServer(ptr);
            addr += Marshal.SizeOf(typeof(IntPtr));
          }
          return new Server();
        } else {
          Console.WriteLine("failed to get servers with type " + type);
          return new Server();
        }
      }

      static void Main(string[] args)
      {
        // this line is necessary for sending an array of pointer structs from go to C#
        // disabling this check is hacky and I don't know the implications of it
        // read more https://golang.org/cmd/cgo/#hdr-Passing_pointers
        // if this doesnt work, run with GODEBUG=cgocheck=0 dotnet run
        string debugEnv = Environment.GetEnvironmentVariable("GODEBUG");
        if (String.IsNullOrEmpty(debugEnv)) {
          throw new Exception("pitaya-cluster lib require you to set env var GODEBUG=cgocheck=0");
        }
        // TODO y the fuck this doesnt work
        Environment.SetEnvironmentVariable("GODEBUG", "cgocheck=0");
        Console.WriteLine("c# prog running");

        GoString[] endpoints = new GoString[]{GoString.fromString("127.0.0.1:2379")};
        int res = Init(GoSlice.fromSlice(endpoints), 30, GoString.fromString("pitaya/"), 30, true, 60, new Server("someid", "game", "{\"ip\":\"127.0.0.1\"}", true));

        Console.WriteLine("res: " + res);

        Console.ReadKey();

        Server sv = GetServer("someid");
        Console.WriteLine("getsv res: " + sv.metadata);

        Console.ReadKey();

        GetServers("game");

        Console.ReadKey();
      }
      
      [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern int Init(GoSlice endpoints, int etcdDialTimeoutSec, GoString etcdPrefix, int heartbeatTTLSec, bool logHeartbeat, int syncServersIntervalSec, Server server);
      
      [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
      static extern PtrResWithStatus GetServerInternal(GoString id);

      [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServersByType")]
      static extern PtrResWithStatus GetServersByTypeInternal(GoString type);

      [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
      static extern void FreeServer(IntPtr ptr);
    }
}
