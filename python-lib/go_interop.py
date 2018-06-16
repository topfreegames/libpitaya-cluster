from ctypes import *

lib = cdll.LoadLibrary("./libpitaya_cluster.dylib")

#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
#  public static extern bool Shutdown();

class SdConfig(Structure):
  _fields_ = [
    ("endpoints", c_char_p),
    ("endpoints_len", c_int),
    ("etcd_dial_timeout_sec", c_int),
    ("etcd_prefix", c_char_p),
    ("heartbeat_ttl_sec", c_int),
    ("log_heartbeat", c_bool),
    ("sync_servers_interval_sec", c_int)]

class NatsRpcClientConfig(Structure):
  _fields_ = [
    ("endpoint", c_char_p),
    ("max_connection_retries", c_int),
    ("request_timeout_ms", c_int)]

class NatsRpcServerConfig(Structure):
  _fields_ = [
    ("endpoint", c_char_p),
    ("max_connection_retries", c_int),
    ("messages_buffer_size", c_int),
    ("rpc_handle_worker_num", c_int)]

class Server(Structure):
  _fields_ = [
    ("id", c_char_p),
    ("type", c_char_p),
    ("metadata", c_char_p),
    ("frontend", c_bool)]

class GoString(Structure):
    _fields_ = [("p", c_char_p), ("n", c_longlong)]

class GetServerRes(Structure):
  _fields_ = [
    ("server", POINTER(Server)),
    ("success", c_bool)]

# Init
lib.Init.restype = c_bool
lib.Init.argtypes = [SdConfig, NatsRpcClientConfig, NatsRpcServerConfig, Server]

#GetServer
lib.GetServer.restype = POINTER(GetServerRes)
lib.GetServer.argtypes= [GoString]

#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServersByType")]
#  static extern IntPtr GetServersByTypeInternal(GoString type);
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
#  static extern IntPtr SendRPC(GoString svId, Route route, GoSlice message);
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
#  public static extern void FreeServer(IntPtr ptr);
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
#  static extern void FreeRPCRes(IntPtr ptr);
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetRPCCallback")]
#  static extern void SetRPCCallbackInternal(IntPtr funcPtr);
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "SetFreeRPCCallback")]
#  static extern void SetFreeRPCCallbackInternal(IntPtr funcPtr);