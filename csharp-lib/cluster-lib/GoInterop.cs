using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Pitaya
{

  [StructLayout(LayoutKind.Sequential)]
  public struct Server
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public string id;
    [MarshalAs(UnmanagedType.LPStr)]
    public string type;
    [MarshalAs(UnmanagedType.LPStr)]
    public string metadata;
    [MarshalAs(UnmanagedType.LPStr)]
    public string hostname;
    public bool frontend;

    public Server(string id, string type, string metadata, string hostname, bool frontend)
    {
      this.id = id;
      this.type = type;
      this.metadata = metadata;
      this.hostname = hostname;
      this.frontend = frontend;
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct SDConfig
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public string endpoints;
    public int etcdDialTimeoutSec;
    [MarshalAs(UnmanagedType.LPStr)]
    public string etcdPrefix;
    public int heartbeatTTLSec;
    public bool logHeartbeat;
    public int syncServersIntervalSec;

    public SDConfig(string endpoints, int etcdDialTimeoutSec, string etcdPrefix, int heartbeatTTLSec, bool logHeartbeat, int syncServersIntervalSec)
    {
      this.endpoints = endpoints;
      this.etcdDialTimeoutSec = etcdDialTimeoutSec;
      this.etcdPrefix = etcdPrefix;
      this.heartbeatTTLSec = heartbeatTTLSec;
      this.logHeartbeat = logHeartbeat;
      this.syncServersIntervalSec = syncServersIntervalSec;
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct Route
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public string svType;
    [MarshalAs(UnmanagedType.LPStr)]
    public string service;
    [MarshalAs(UnmanagedType.LPStr)]
    public string method;

    public Route(string svType, string service, string method)
    {
      this.svType = svType;
      this.service = service;
      this.method = method;
    }

    public static Route fromString(string r)
    {
      string[] res = r.Split(new string[] { "." }, StringSplitOptions.None);
      if (res.Length < 3)
      {
        throw new Exception(String.Format("invalid route: {0}", r));
      }
      return new Route(res[0], res[1], res[2]);
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct RPCReq
  {
    public IntPtr data;
    public int dataLen;
    [MarshalAs(UnmanagedType.LPStr)]
    public string route;

    public byte[] getReqData()
    {
      byte[] data = new byte[this.dataLen];
      Marshal.Copy(this.data, data, 0, this.dataLen);
      return data;
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct RPCRes
  {
    public IntPtr data;
    public int dataLen;

    public byte[] getResData()
    {
      byte[] data = new byte[this.dataLen];
      Marshal.Copy(this.data, data, 0, this.dataLen);
      return data;
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct NatsConfig
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public string endpoint;
    public Int64 connectionTimeoutMs;
    public int requestTimeoutMs;
    public int maxConnectionRetries;
    public int maxPendingMessages;

    public NatsConfig(string endpoint, int connectionTimeoutMs, int requestTimeoutMs, int maxConnectionRetries, int maxPendingMessages)
    {
      this.endpoint = endpoint;
      this.connectionTimeoutMs = connectionTimeoutMs;
      this.requestTimeoutMs = requestTimeoutMs;
      this.maxConnectionRetries = maxConnectionRetries;
      this.maxPendingMessages = maxPendingMessages;
    }
  }

  public struct GrpcRPCClientConfig
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public string etcdEndpoints;
    public string etcdPrefix;
    public int requestTimeoutMs;
    public int dialTimeoutMs;
    public int etcdDialTimeoutMs;
    public int etcdLeaseTTLS;

    public GrpcRPCClientConfig(int requestTimeoutMs, int dialTimeoutMs, string etcdEndpoints, string etcdPrefix, int etcdDialTimeoutMs = 10000, int etcdLeaseTTLS = 60)
    {
      this.requestTimeoutMs = requestTimeoutMs;
      this.dialTimeoutMs = dialTimeoutMs;
      this.etcdEndpoints = etcdEndpoints;
      this.etcdPrefix = etcdPrefix;
      this.etcdDialTimeoutMs = etcdDialTimeoutMs;
      this.etcdLeaseTTLS = etcdLeaseTTLS;
    }
  }

  public struct GrpcRPCServerConfig
  {
    [MarshalAs(UnmanagedType.LPStr)]
    public int port;

    public GrpcRPCServerConfig(int port)
    {
      this.port = port;
    }
  }
}

class StructWrapper : IDisposable
{
  public IntPtr Ptr { get; private set; }

  public StructWrapper(object obj)
  {
    if (Ptr != null)
    {
      Ptr = Marshal.AllocHGlobal(Marshal.SizeOf(obj));
      Marshal.StructureToPtr(obj, Ptr, false);
    }
    else
    {
      Ptr = IntPtr.Zero;
    }
  }

  ~StructWrapper()
  {
    if (Ptr != IntPtr.Zero)
    {
      Marshal.FreeHGlobal(Ptr);
      Ptr = IntPtr.Zero;
    }
  }

  public void Dispose()
  {
    Marshal.FreeHGlobal(Ptr);
    Ptr = IntPtr.Zero;
    GC.SuppressFinalize(this);
  }

  public static implicit operator IntPtr(StructWrapper w)
  {
    return w.Ptr;
  }
}
