using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using Google.Protobuf;
using System.Collections.Generic;

namespace Pitaya
{
  public class PitayaCluster : IDisposable
  {
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate IntPtr RPCCb(IntPtr req);
    public delegate string RemoteNameFunc(string methodName);

    public delegate void OnSignalFunc();

    public delegate void LogHandler(string msg);

    public delegate void SignalHandler();

    private Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    private static Action onSignalEvent;

    private IntPtr _clusterPtr;

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

    public void RegisterRemote(BaseRemote remote)
    {
      string className = remote.GetType().Name.ToLower();
      RegisterRemote(remote, className, Utils.DefaultRemoteNameFunc);
    }

    public void RegisterRemote(BaseRemote remote, string name)
    {
      RegisterRemote(remote, name, Utils.DefaultRemoteNameFunc);
    }

    public void RegisterRemote(BaseRemote remote, string name, RemoteNameFunc remoteNameFunc)
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
    private IntPtr RPCCbFunc(IntPtr reqPtr)
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
      Logger.Debug("resData is: " + Encoding.UTF8.GetString(resData));
      var response = new Protos.Response();
      response.MergeFrom(new CodedInputStream(resData));
      var res = (IMessage)Activator.CreateInstance(typeof(T));
      res.MergeFrom(new CodedInputStream(resData));
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

    public PitayaCluster(SDConfig sdConfig, NatsConfig natsCfg, Server server, LogHandler logHandler = null)
    {
      Logger.Debug("SYNC SERVERS: " + sdConfig.syncServersIntervalSec);
      IntPtr sdCfgPtr = new StructWrapper(sdConfig);
      IntPtr natsCfgPtr = new StructWrapper(natsCfg);
      IntPtr serverPtr = new StructWrapper(server);

      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);
      IntPtr logFuncPtr = logHandler != null ? Marshal.GetFunctionPointerForDelegate(logHandler) : IntPtr.Zero;
      _clusterPtr = NewClusterInternal(serverPtr, sdCfgPtr, natsCfgPtr, rpcCbFuncPtr, logFuncPtr);

      if (_clusterPtr == IntPtr.Zero)
      {
        throw new PitayaException("Initialization failed");
      }
    }

    ~PitayaCluster() => Dispose();

    public void Dispose()
    {
      if (_clusterPtr != IntPtr.Zero)
        DestroyClusterInternal(_clusterPtr);
      _clusterPtr = IntPtr.Zero;
    }

    public Server? GetServerById(string serverId)
    {
      IntPtr serverPtr = GetServerByIdInternal(_clusterPtr, serverId);

      if (serverPtr == IntPtr.Zero)
      {
        Logger.Error("There are no servers with id " + serverId);
        return null;
      }

      var server = (Pitaya.Server)Marshal.PtrToStructure(serverPtr, typeof(Pitaya.Server));
      FreeServerInternal(serverPtr);
      return server;
    }

    public unsafe T Rpc<T>(string serverId, Route route, IMessage msg)
    {
      // IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(MemoryBuffer)));

      IntPtr pitayaErr = IntPtr.Zero;
      MemoryBuffer* memBufPtr = null;

      var data = ProtoMessageToByteArray(msg);
      fixed (byte* p = data)
      {
        pitayaErr = RPCInternal(_clusterPtr, serverId, route.ToString(), (IntPtr)p, data.Length, &memBufPtr);
      }

      if (pitayaErr != IntPtr.Zero) // error
      {
        var err = (Pitaya.Error)Marshal.PtrToStructure(pitayaErr, typeof(Pitaya.Error));

        FreePitayaErrorInternal(pitayaErr);

        throw new PitayaException($"RPC call failed: ({err.code}: {err.msg})");
      }

      var protoRet = GetProtoMessageFromMemoryBuffer<T>(*memBufPtr);
      FreeMemoryBufferInternal(memBufPtr);
      return protoRet;
    }

    public T Rpc<T>(Route route, IMessage msg)
    {
      return Rpc<T>("", route, msg);
    }


    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_NewCluster")]
    private static extern IntPtr NewClusterInternal(IntPtr server, IntPtr sdConfig, IntPtr natsCfg, IntPtr cbPtr, IntPtr logHandler);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_DestroyCluster")]
    private static extern void DestroyClusterInternal(IntPtr clusterPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_GetServerById")]
    private static extern IntPtr GetServerByIdInternal(IntPtr clusterPtr, string serverId);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeServer")]
    private static extern void FreeServerInternal(IntPtr serverPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_RPC")]
    private static extern unsafe IntPtr RPCInternal(IntPtr clusterPtr, string serverId, string route, IntPtr data, int dataSize, MemoryBuffer** buffer);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeMemoryBuffer")]
    private static extern unsafe void FreeMemoryBufferInternal(MemoryBuffer *ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_OnSignal")]
    private static extern void OnSignalInternal(OnSignalFunc ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreePitayaError")]
    private static extern void FreePitayaErrorInternal(IntPtr ptr);
  }
}
