from ctypes import *

lib = cdll.LoadLibrary("./libpitaya_cluster.dylib")

#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl)]
#  public static extern bool Shutdown();
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "Init")]
#  static extern bool InitInternal(SDConfig sdConfig, NatsRPCClientConfig rpcClientConfig, NatsRPCServerConfig rpcServerConfig, Server server);

class sd_config(Structure):
  _fields_ = [
    ("endpoints", POINTER(c_char_p)),
    ("endpoints_len", c_int),
    ("etcd_dial_timeout_sec", c_int),
    ("etcd_prefix", c_char_p),
    ("heartbeat_ttl_sec", c_int),
    ("log_heartbeat", c_bool),
    ("sync_servers_interval_sec", c_int)]

class nats_rpc_client_config(Structure):
  _fields_ = [
    ("endpoint", c_char_p),
    ("max_connection_retries", c_int),
    ("request_timeout_ms", c_int)]

class nats_rpc_server_config(Structure):
  _fields_ = [
    ("endpoint", c_char_p),
    ("max_connection_retries", c_int),
    ("messages_buffer_size", c_int),
    ("rpc_handle_worker_num", c_int)]

class server(Structure):
  _fields_ = [
    ("id", c_char_p),
    ("type", c_char_p),
    ("metadata", c_char_p),
    ("frontend", c_bool)]

def py_to_c_str_array(py_str_array):
  num_str = len(py_str_array)
  str_array_type = c_char_p * num_str
  str_array = str_array_type()
  for i, param in enumerate(py_str_array):
    str_array[i] = param
  return str_array

lib.Init.restype = c_bool
lib.Init.argtypes = [sd_config, nats_rpc_client_config, nats_rpc_server_config, server]
#
#[DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint= "GetServer")]
#  static extern IntPtr GetServerInternal(GoString id);
#
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