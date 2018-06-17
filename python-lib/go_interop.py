from ctypes import *

lib = cdll.LoadLibrary("./libpitaya_cluster.dylib")

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

class RPCRes(Structure):
  _fields_ = [
    ("data", c_void_p),
    ("data_len", c_int)]

class RPCReq(Structure):
  _fields_ = [
    ("data", c_void_p),
    ("data_len", c_int),
    ("route", c_char_p)]

class GoString(Structure):
  _fields_ = [("p", c_char_p), ("n", c_longlong)]

class GoSlice(Structure):
  _fields_ = [("data", c_void_p), ("len", c_longlong), ("cap", c_longlong)]

class Route(Structure):
  _fields_ = [("sv_type", c_char_p), ("service", c_char_p), ("method", c_char_p)]

def route_from_str(route_str:str):
  r = Route()
  route_splitted = route_str.split(".")
  if len(route_splitted) != 3:
    raise Exception('invalid route {}'.format(route_str))
  r.sv_type = route_splitted[0].encode('utf-8')
  r.service = route_splitted[1].encode('utf-8')
  r.method = route_splitted[2].encode('utf-8')
  return r

RPCCB = CFUNCTYPE(c_void_p, RPCReq)

lib.Init.restype = c_bool
lib.Init.argtypes = [SdConfig, NatsRpcClientConfig, NatsRpcServerConfig, Server]

lib.GetServer.restype = c_bool
lib.GetServer.argtypes = [GoString, POINTER(Server)]

lib.FreeServer.argtypes = [POINTER(Server)]

lib.FreeRPCRes.argtypes = [POINTER(RPCRes)]

lib.Shutdown.restype = c_bool

lib.SendRPC.restype = c_bool
lib.SendRPC.argtypes = [GoString, Route, GoSlice, POINTER(RPCRes)]

lib.SetRPCCallback.argtypes = [c_void_p]