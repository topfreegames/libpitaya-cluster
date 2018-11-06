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

    public delegate void LogHandler(string msg);

    private static Dictionary<string, RemoteMethod> remotesDict = new Dictionary<string, RemoteMethod>();

    public static Action onSignalEvent;

    public static Protos.Response GetErrorResponse(string code, string msg)
    {
      var response = new Protos.Response {Error = new Protos.Error {Code = code, Msg = msg}};
      return response;
    }

    public static byte[] ProtoMessageToByteArray(IMessage msg)
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
        var remoteName = $"{name}.{rn}";
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

    private static void OnSignal()
    {
      Logger.Info("================================================");
      Logger.Info("Recevied SIGNAL FROM C++ :D");
      Logger.Info("================================================");
      if (onSignalEvent != null)
        onSignalEvent();
    }

    public static bool InitDefault(SDConfig sdConfig, NatsConfig natsCfg, Server server, LogHandler logHandler = null)
    {
      IntPtr sdCfgPtr = new StructWrapper(sdConfig);
      IntPtr natsCfgPtr = new StructWrapper(natsCfg);
      IntPtr serverPtr = new StructWrapper(server);

      PitayaCluster.RPCCb rpcCbFunc = RPCCbFunc;
      IntPtr rpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(rpcCbFunc);
      IntPtr logFuncPtr = logHandler != null ? Marshal.GetFunctionPointerForDelegate(logHandler) : IntPtr.Zero;
      bool success = InitializeInternal(serverPtr, sdCfgPtr, natsCfgPtr, rpcCbFuncPtr, logFuncPtr);
      OnSignalInternal(OnSignal);
      return success;
    }

    public static bool Init(SDConfig sdConfig, NatsConfig natsCfg, Server server, LogHandler handler = null)
    {
      return InitDefault(sdConfig, natsCfg, server, handler);
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

    public static unsafe T Rpc<T>(string serverId, Route route, IMessage msg)
    {
      // IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(MemoryBuffer)));

      IntPtr pitayaErr = IntPtr.Zero;
      MemoryBuffer* memBufPtr = null;

      byte[] data = ProtoMessageToByteArray(msg);
      fixed (byte* p = data)
      {
        pitayaErr = RPCInternal(serverId, route.ToString(), (IntPtr)p, data.Length, &memBufPtr);
      }

      if (pitayaErr != IntPtr.Zero) // error
      {
        var err = (Pitaya.Error)Marshal.PtrToStructure(pitayaErr, typeof(Pitaya.Error));

        FreePitayaErrorInternal(pitayaErr);

        throw new Exception($"RPC call failed: ({err.code}: {err.msg})");
      }

      var protoRet = GetProtoMessageFromMemoryBuffer<T>(*memBufPtr);
      FreeMemoryBufferInternal(memBufPtr);
      return protoRet;
    }

    public static T Rpc<T>(Route route, IMessage msg)
    {
      return Rpc<T>("", route, msg);
    }


    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Initialize")]
    private static extern unsafe bool InitializeInternal(IntPtr server, IntPtr sdConfig, IntPtr natsCfg, IntPtr cbPtr, IntPtr logHandler);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Shutdown")]
    private static extern void ShutdownInternal();

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_GetServerById")]
    private static extern IntPtr GetServerByIdInternal(string serverId);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeServer")]
    private static extern void FreeServerInternal(IntPtr serverPtr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_RPC")]
    private static extern unsafe IntPtr RPCInternal(string serverId, string route, IntPtr data, int dataSize, MemoryBuffer** buffer);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeMemoryBuffer")]
    private static extern unsafe void FreeMemoryBufferInternal(MemoryBuffer *ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_OnSignal")]
    private static extern void OnSignalInternal(OnSignalFunc ptr);

    [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreePitayaError")]
    private static extern void FreePitayaErrorInternal(IntPtr ptr);
  }
}
