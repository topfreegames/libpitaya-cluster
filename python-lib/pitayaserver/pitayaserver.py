from .route import Route
from google.protobuf import message
from ctypes import POINTER, byref, c_char, c_char_p, c_void_p, addressof, memmove, sizeof
from .gen.response_pb2 import Response
from .gen.request_pb2 import Request
from .remote import BaseRemote
from .c_interop import SdConfig, NatsConfig, Server, Native, FREECB, MemoryBuffer, RPCReq, RPCCB, PitayaError, LogLevel

from multiprocessing import cpu_count
from threading import Thread

remotes_dict = dict()
rpc_worker_threads = []

def default_name_func(s): return s[:1].lower() + s[1:] if s else ''

LIB = None

class UnsafeResponseData:
    __data_ptr = None
    response_ptr = None

    def __init__(self, res:Response):
        """ internal method, this allocs a memory buffer in the global heap for sending it to c code """
        res_bytes = res.SerializeToString()
        ret_len = len(res_bytes)
        self.__data_ptr = LIB.tfg_pitc_AllocMem(ret_len)
        ret_data = (c_char * ret_len)(*res_bytes)
        memmove(self.__data_ptr, ret_data, ret_len)
        ret = MemoryBuffer()
        ret.data = self.__data_ptr
        ret.size = ret_len
        # we alloc mem in c side because doing so from python causes it to be freed the moment we return
        self.response_ptr = LIB.tfg_pitc_AllocMem(sizeof(ret))
        memmove(self.response_ptr, addressof(ret), sizeof(ret))
    
    def __del__(self):
        LIB.tfg_pitc_FreeMem(self.__data_ptr)
        LIB.tfg_pitc_FreeMem(self.response_ptr)


def initialize_pitaya(
        sd_config: SdConfig,
        nats_config: NatsConfig,
        server: Server, log_level: LogLevel, logPath=b'/tmp/pitaya_cluster_log'):
    """ this method initializes pitaya cluster logic and should be called on start """
    global LIB
    LIB = Native().LIB
    res = LIB.tfg_pitc_InitializeWithNats(
        nats_config, sd_config, server, log_level, logPath)
    if not res:
        raise Exception("error initializing pitaya")
    # initialize RPC threads
    for i in range(cpu_count()):
        t = Thread(target=process_rpcs, args=(i,))
        rpc_worker_threads.append(t)
        t.start()


def process_rpcs(thread_num):
    print (f"started working thread {thread_num}")
    while True:
        cRpcPtr = LIB.tfg_pitc_WaitForRpc()
        if bool(cRpcPtr) == False:
            break
        req = cRpcPtr.contents
        try:
            res = Response()
            req_data_ptr = req.buffer.contents.data
            req_data_sz = req.buffer.contents.size
            req_data = (c_char * req_data_sz).from_address(req_data_ptr)
            request = Request()
            request.MergeFromString(req_data.value)
            route = Route.from_str(request.msg.route).str()
            if route not in remotes_dict:
                raise Exception("remote %s not found!" % route)
            remote_method = remotes_dict[route]
            arg = remote_method.arg_type()
            arg.MergeFromString(request.msg.data)
            ans = remote_method.method(remote_method.obj, arg)
            res.data = ans.SerializeToString()
            r = UnsafeResponseData(res)
            LIB.tfg_pitc_FinishRpcCall(r.response_ptr, cRpcPtr)
        except Exception as e:
            err_str = "exception: %s: %s" % (type(e).__name__, e)
            r = _get_error_response_c_void_p("PIT-500", err_str)
            LIB.tfg_pitc_FinishRpcCall(r.response_ptr, cRpcPtr)
            continue


def get_server_by_id(server_id: str):
    """ gets a server by its id """
    # TODO needs free -- leak here!
    sv = Server()
    success = LIB.tfg_pitc_GetServerById(server_id.encode('utf-8'), sv)
    if success is False:
        raise Exception('failed to get server {}'.format(server_id))
    # LIB.tfg_pitc_FreeServer(byref(sv))
    return sv


def register_remote(remote: BaseRemote, name="", name_func=None):
    """ register a remote, the remote should be a class that inherits from BaseRemote """
    if name == "":
        name = remote.__class__.__name__.lower()
    if not issubclass(type(remote), BaseRemote):
        raise TypeError
    if not remote.is_valid_remote():
        raise Exception("the class %s contains no valid remote methods" %
                        remote.__class__.__name__)
    m_map = remote.get_remotes_map()
    for m in m_map.keys():
        rn = m
        if name_func is None:
            name_func = default_name_func
        rn = name_func(rn)
        name = "%s.%s" % (name, rn)
        if name in remotes_dict:
            raise Exception("tried to register same remote twice! %s" % name)
        # TODO better logger
        print("registered remote: %s" % name)
        remotes_dict[name] = m_map[m]


def shutdown():
    """ shutdown pitaya cluster, should be called on exit """
    LIB.tfg_pitc_Terminate()


def _get_error_response_c_void_p(code, msg):
    """ gets an allocated buffer of an error response """
    res = Response()
    res.error.code = code
    res.error.msg = msg
    return UnsafeResponseData(res)


def send_rpc(route: str, in_msg: message.Message, res_class: message.Message, server_id: str='') -> message.Message:
    """ sends a rpc to other pitaya server """
    if not issubclass(type(in_msg), message.Message) or not issubclass(res_class, message.Message):
        raise TypeError
    msg_bytes = in_msg.SerializeToString()
    msg_len = len(msg_bytes)
    c_bytes = (c_char * msg_len)(*msg_bytes)
    ret_ptr = POINTER(MemoryBuffer)()
    err = PitayaError()
    res = LIB.tfg_pitc_RPC(server_id.encode(
        'utf-8'), route.encode('utf-8'), addressof(c_bytes), msg_len, byref(ret_ptr), byref(err))
    if not res:
        exception_msg = "code: {} msg: {}".format(err.code, err.msg)
        LIB.tfg_pitc_FreePitayaError(err)
        raise Exception(exception_msg)
    ret_bytes = (
        c_char * ret_ptr.contents.size).from_address(ret_ptr.contents.data)
    response = Response()
    response.MergeFromString(ret_bytes.value)
    res = res_class()
    res.MergeFromString(response.data)
    LIB.tfg_pitc_FreeMemoryBuffer(ret_ptr)
    return res

# MUST be called after get_server or else your code will leak
# TODO: can we prevent the user from not calling this?
# def free_server(server:Server):
#    LIB.tfg_pitc_FreeServer(server)
#
