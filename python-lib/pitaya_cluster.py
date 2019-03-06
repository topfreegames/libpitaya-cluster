import c_interop as pc
from remote import ExampleRemote, BaseRemote
import gen.cluster_pb2 as cluster_pb
from route import Route

from uuid import *
from google.protobuf import message
from ctypes import *
from gen.response_pb2 import Response
from gen.error_pb2 import Error
from time import sleep

remotes_dict = dict()

def init(
    sd_config:pc.SdConfig,
    nats_config:pc.NatsConfig,
    server:pc.Server):
    ## TODO fix logpath
    return pc.LIB.tfg_pitc_Initialize(server, sd_config, nats_config, rpc_cb, free_cb, b'/tmp/log')

def get_server_by_id(server_id:str):
    ## TODO needs free
    sv = pc.Server()
    success = pc.LIB.tfg_pitc_GetServerById(server_id.encode('utf-8'), sv)
    if success == False:
        raise Exception('failed to get server {}'.format(server_id))
    return sv

def register_remote(remote:BaseRemote, name="", nameFunc=None):
    if name == "":
        name = remote.__class__.__name__.lower()
    if not issubclass(type(remote), BaseRemote):
        raise TypeError
    if not remote.IsValidRemote():
        raise Exception("the class %s contains no valid remote methods" % remote.__class__.__name__)
    m_map = remote.GetRemotesMap()
    for m in m_map.keys():
        rn = m
        if nameFunc == None:
            nameFunc = defaultNameFunc
        rn = nameFunc(rn)
        name = "%s.%s" % (name, rn)
        if name in remotes_dict:
            raise Exception("tried to register same remote twice! %s" % name)
        ## TODO better logger
        print("registered remote: %s" % name)
        remotes_dict[name] = m_map[m]

def shutdown():
    pc.LIB.tfg_pitc_Terminate()

@pc.FREECB
def free_cb(mem:c_void_p):
    pc.LIB.tfg_pitc_FreeMem(mem)

def alloc_mem_buffer_ptr_with_response_data(res:Response):
    if not isinstance(res, Response):
        raise TypeError
    res_bytes = res.SerializeToString()
    ret_len = len(res_bytes)
    ptrData = pc.LIB.tfg_pitc_AllocMem(ret_len) 
    ret_data = (c_char * ret_len)(*res_bytes)
    memmove(ptrData, ret_data, ret_len) 
    ret = pc.MemoryBuffer()
    ret.data = ptrData
    ret.size = ret_len
    #we alloc mem in c side because doing so from python causes it to be freed the moment we return
    ptrStruct = pc.LIB.tfg_pitc_AllocMem(sizeof(ret)) 
    memmove(ptrStruct, addressof(ret), sizeof(ret))
    return ptrStruct

def get_error_response_c_void_p(code, msg):
    res = Response()
    res.error.code = code
    res.error.msg = msg
    return alloc_mem_buffer_ptr_with_response_data(res)

@pc.RPCCB
def rpc_cb(req:POINTER(pc.RPCReq)) -> c_void_p:
    r = req.contents.route
    route = Route.from_str(r).str()
    if not route in remotes_dict:
        return get_error_response_c_void_p("PIT-404", "remote %s not found!" % route)
    remote_method = remotes_dict[route]
    try:
        res = Response()
        req_data_ptr = req.contents.buffer.data
        req_data_sz = req.contents.buffer.size
        req_data = (c_char * req_data_sz).from_address(req_data_ptr)
        arg = remote_method.arg_type()
        arg.MergeFromString(req_data)
        ans = remote_method.method(remote_method.obj, arg)
        res.data = ans.SerializeToString()
        return alloc_mem_buffer_ptr_with_response_data(res)
    except Exception as e:
        err_str = "exception: %s: %s" % (type(e).__name__, e)
        return get_error_response_c_void_p("PIT-500", err_str)

def send_rpc(route:str, in_msg:message.Message, res_class:message.Message, server_id:str='') -> message.Message:
    if not issubclass(type(in_msg), message.Message) or not issubclass(res_class, message.Message):
        raise TypeError
    msg_bytes = in_msg.SerializeToString()
    msg_len = len(msg_bytes)
    svId = server_id.encode('utf-8')
    c_bytes = (c_char * msg_len)(*msg_bytes)
    ret_ptr = POINTER(pc.MemoryBuffer)()
    err = pc.PitayaError()
    res = pc.LIB.tfg_pitc_RPC(server_id.encode('utf-8'), route.encode('utf-8'), addressof(c_bytes), msg_len, byref(ret_ptr), byref(err))
    if not res:
        ## TODO: needs free
        raise Exception("code: {} msg: {}".format(err.code, err.msg))
    ret_bytes = (c_char * ret_ptr.contents.size).from_address(ret_ptr.contents.data)
    response = Response()
    response.MergeFromString(ret_bytes)
    pc.LIB.tfg_pitc_FreeMemoryBuffer(ret_ptr)
    out_msg = res_class()
    out_msg.MergeFromString(response.data)
    return out_msg

# MUST be called after get_server or else your code will leak
# TODO: can we prevent the user from not calling this?
#def free_server(server:pc.Server):
#    pc.LIB.tfg_pitc_FreeServer(server)
#

def run():
    sv_id = uuid4().hex
    res = init(
        pc.SdConfig(b'http://127.0.0.1:4001', b'pitaya/', 30, True, True, True, 60, pc.LOGLEVEL_DEBUG),
        pc.NatsConfig(b'127.0.0.1:4222', 5, 5000, 5, 100),
        pc.Server(sv_id.encode('utf-8'), b'python', b'{}', b'localhost', False),
    )
    if not res:
        print('error initializing pitaya :/')
        return
    print('successfully initialized pitaya!')
    sv = get_server_by_id(sv_id)
    print('got sv!', sv.type, sv.id)
    ## TODO try to use destructor
    #free_server(sv)

    rpc_msg = cluster_pb.RPCMsg()
    rpc_msg.Msg = "hello from python"

    ## wait for server to be registered
    sleep(1)

    ret = send_rpc("python.exampleRemote.testRemote", rpc_msg, cluster_pb.RPCRes)
    print("got rpc res {}".format(ret))
    
    input('Press enter to exit')
    shutdown()


defaultNameFunc = lambda s: s[:1].lower() + s[1:] if s else ''

b = ExampleRemote()
register_remote(b, "exampleRemote", defaultNameFunc)

run()
