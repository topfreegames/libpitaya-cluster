from ctypes import *

LIB = cdll.LoadLibrary("../precompiled/libpitaya_cluster.dylib")

class SdConfig(Structure):
    _fields_ = [
        ("endpoints", c_char_p),
        ("etcd_prefix", c_char_p),
        ("heartbeat_ttl_sec", c_int),
        ("log_heartbeat", c_int),
        ("log_server_sync", c_int),
        ("log_server_details", c_int),
        ("sync_servers_interval_sec", c_int),
        ("log_level", c_int)]

(LOGLEVEL_DEBUG, LOGLEVEL_INFO, LOGLEVEL_WARN, LOGLEVEL_ERROR,
 LOGLEVEL_CRITICAL) = (0, 1, 2, 3, 4)

class NatsConfig(Structure):
    _fields_ = [
        ("endpoint", c_char_p),
        ("connection_timeout_ms", c_longlong),
        ("request_timeout_ms", c_int),
        ("max_reconnection_attempts", c_int),
        ("max_pending_msgs", c_int)]

class Server(Structure):
    _fields_ = [
        ("id", c_char_p),
        ("type", c_char_p),
        ("metadata", c_char_p),
        ("hostname", c_char_p),
        ("frontend", c_int)]

class PitayaError(Structure):
    _fields_ = [
        ("code", c_char_p),
        ("msg", c_char_p)]

class MemoryBuffer(Structure):
    _fields_ = [
        ("data", c_void_p),
        ("size", c_int)]

class RPCReq(Structure):
    _fields_ = [
        ("buffer", MemoryBuffer),
        ("route", c_char_p)]

RPCCB = CFUNCTYPE(c_void_p, POINTER(RPCReq))
FREECB = CFUNCTYPE(None, c_void_p)

LIB.tfg_pitc_Initialize.restype = c_bool
LIB.tfg_pitc_Initialize.argtypes = [POINTER(Server), POINTER(SdConfig), POINTER(NatsConfig),
                                    RPCCB, FREECB, c_char_p]

LIB.tfg_pitc_GetServerById.restype = c_bool
LIB.tfg_pitc_GetServerById.argtypes = [c_char_p, POINTER(Server)]

#LIB.tfg_pitc_FreeServer.argtypes = [POINTER(Server)]

LIB.tfg_pitc_Terminate.restype = None

LIB.tfg_pitc_FreeMemoryBuffer.argtypes = [POINTER(MemoryBuffer)]

LIB.tfg_pitc_FreePitayaError.argtypes = [POINTER(PitayaError)]

LIB.tfg_pitc_RPC.restype = c_bool
LIB.tfg_pitc_RPC.argtypes = [c_char_p, c_char_p, c_void_p, c_int,
                             POINTER(POINTER(MemoryBuffer)), POINTER(PitayaError)]
