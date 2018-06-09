using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Pitaya
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

  public struct SDConfig {
    [MarshalAs(UnmanagedType.ByValArray)]
    public string[] endpoints;
    public int endpointsLen;
    public int etcdDialTimeoutSec;
    [MarshalAs(UnmanagedType.LPStr)]
    public string etcdPrefix;
    public int heartbeatTTLSec;
    public bool logHeartbeat;
    public int syncServersIntervalSec;

    public SDConfig(string[] endpoints, int etcdDialTimeoutSec, string etcdPrefix, int heartbeatTTLSec, bool logHeartbeat, int syncServersIntervalSec) {
      this.endpoints = endpoints;
      this.endpointsLen = endpoints.Length;
      this.etcdDialTimeoutSec = etcdDialTimeoutSec;
      this.etcdPrefix = etcdPrefix;
      this.heartbeatTTLSec = heartbeatTTLSec;
      this.logHeartbeat = logHeartbeat;
      this.syncServersIntervalSec = syncServersIntervalSec;
    }
  }

  public struct Route {
    [MarshalAs(UnmanagedType.LPStr)]
    public string svType;
    [MarshalAs(UnmanagedType.LPStr)]
    public string service;
    [MarshalAs(UnmanagedType.LPStr)]
    public string method;

    public Route(string svType, string service, string method) {
      this.svType = svType;
      this.service = service;
      this.method = method;
    }

    public static Route fromString(string r){
      string[] res = r.Split(new string[]{"."}, StringSplitOptions.None);
      if (res.Length < 3) {
        throw new Exception(String.Format("invalid route: {0}", r));
      }
      return new Route(res[0], res[1], res[2]);
    }
  }

  public struct RPCReq {
    public IntPtr data;
    public int dataLen;
    [MarshalAs(UnmanagedType.LPStr)]
    public string route;

    public byte[] getReqData() {
      byte[] data = new byte[this.dataLen];
      Marshal.Copy(this.data, data, 0, this.dataLen);
      return data;
    }
  }

  // TODO receive error
  public struct RPCRes {
    public IntPtr data;
    public int dataLen;
    public bool success;

    public byte[] getResData() {
      byte[] data = new byte[this.dataLen];
      Marshal.Copy(this.data, data, 0, this.dataLen);
      return data;
    }
  }

  public struct Error {
    public GoString Msg;
    public GoString Code;
  }

  public struct NatsRPCClientConfig {
    [MarshalAs(UnmanagedType.LPStr)]
    public string endpoint;
    public int maxConnectionRetries;
    public int requestTimeoutMs;

    public NatsRPCClientConfig(string endpoint, int maxConnectionRetries, int requestTimeoutMs) {
      this.endpoint = endpoint;
      this.maxConnectionRetries = maxConnectionRetries;
      this.requestTimeoutMs = requestTimeoutMs;
    }
  }

  public struct NatsRPCServerConfig {
    [MarshalAs(UnmanagedType.LPStr)]
    public string endpoint;
    public int maxConnectionRetries;
    public int messagesBufferSize;

    public NatsRPCServerConfig(string endpoint, int maxConnectionRetries, int messagesBufferSize) {
      this.endpoint = endpoint;
      this.maxConnectionRetries = maxConnectionRetries;
      this.messagesBufferSize = messagesBufferSize;
    }
  }

  public struct PtrResWithStatus {
    public IntPtr ptr;
    public int status;

    public PtrResWithStatus(IntPtr ptr, int status) {
      this.ptr = ptr;
      this.status = status;
    }

    public T getStructFromPtr<T>(){
      T res = (T) Marshal.PtrToStructure(this.ptr, typeof(T));
      return res;
    }
  }
}
