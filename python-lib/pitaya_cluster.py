import go_interop as pc
import gen.cluster_pb2 as cluster_pb

from uuid import *
from google.protobuf import message
from ctypes import *
from gen.response_pb2 import Response
from time import sleep

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

def shutdown():
    pc.LIB.tfg_pitc_Terminate()

@pc.FREECB
def free_cb(mem:c_void_p):
    print("free ", mem)

@pc.RPCCB
def rpc_cb(req:POINTER(pc.RPCReq)):
    print("hellow ", req.contents.route)
    invoke_res = cluster_pb.RPCRes()
    invoke_res.Msg = "hellow from python"
    invoke_res.Code = 777
    res = Response()
    res.data = invoke_res.SerializeToString()
    ## TODO populate with error
    res_bytes = res.SerializeToString()
    ret_len = len(res_bytes)
    ## TODO free
    ret_data = (c_char * ret_len)(*res_bytes)

    ## TODO duplicated code can be refactored with send_rpc
    ret = pc.MemoryBuffer()
    ret.data = addressof(ret_data)
    ret.size = ret_len
    print("addresses", addressof(ret), ret.data)
    return addressof(ret)

def send_rpc(route:str, in_msg:message, res_class:message, server_id:str=''):
    msg_bytes = in_msg.SerializeToString()
    msg_len = len(msg_bytes)
    svId = server_id.encode('utf-8')
    ## TODO needs free
    c_bytes = (c_char * msg_len)(*msg_bytes)
    ## TODO needs free
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

    ret = send_rpc("python.testremote.test", rpc_msg, cluster_pb.RPCRes)
    print("got rpc res {}".format(ret))
    
    input('Press enter to exit')
    shutdown()

run()
