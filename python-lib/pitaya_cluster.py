import go_interop as pc
import gen.cluster_pb2 as cluster_pb

from uuid import *
from google.protobuf import message
from ctypes import *
from gen.response_pb2 import Response

def init(
    sd_config:pc.SdConfig,
    nats_rpc_client_config:pc.NatsRpcClientConfig,
    nats_rpc_server_config:pc.NatsRpcServerConfig,
    server:pc.Server):
    return pc.lib.Init(sd_config, nats_rpc_client_config, nats_rpc_server_config, server)

def get_server(server_id:str):
    sv = pc.Server()
    success = pc.lib.GetServer(pc.GoString(server_id.encode('utf-8'), len(server_id)), sv)
    if success == False:
        raise Exception('failed to get server {}'.format(server_id))
    return sv

def shutdown():
    return pc.lib.Shutdown()

def rpc_cb(req:pc.RPCReq):
    print("hellow", req.route)

def send_rpc(route:str, in_msg:message, res_class:message, server_id:str=''):
    r = pc.route_from_str(route)
    msg_bytes = in_msg.SerializeToString()
    msg_len = len(msg_bytes)
    go_sv = pc.GoString(server_id.encode('utf-8'), len(server_id))
    go_bytes = (c_char * msg_len)(*msg_bytes)
    go_msg = pc.GoSlice(addressof(go_bytes), msg_len, msg_len)
    ret = pc.RPCRes()
    res = pc.lib.SendRPC(go_sv, r, go_msg, ret)
    if not res:
        raise Exception('error sending rpc!')
    ret_bytes = (c_char * ret.data_len).from_address(ret.data)
    response = Response()
    response.MergeFromString(ret_bytes)
    pc.lib.FreeRPCRes(ret)
    out_msg = res_type()
    out_msg.MergeFromString(response.data)
    return out_msg

# MUST be called after get_server or else your code will leak
# TODO: can we prevent the user from not calling this?
def free_server(server:pc.Server):
    pc.lib.FreeServer(server)

# MUST be called after send_rpc or else your code will leak
# TODO: can we prevent the user from not calling this?
def free_rpcres(rpcres:pc.RPCRes):
    pc.lib.FreeRPCRes(rpcres)

sv_id = uuid4().hex

res = init(
    pc.SdConfig(b'127.0.0.1:2379', 1, 30, b'pitaya/', 30, True, 60),
    pc.NatsRpcClientConfig(b'nats://127.0.0.1:4222', 10, 5000),
    pc.NatsRpcServerConfig(b'nats://127.0.0.1:4222', 10, 75, 10),
    pc.Server(sv_id.encode('utf-8'), b'python', b'{}', False),
)

if res:
    print('successfully initialized pitaya!')

sv = get_server(sv_id)
print('got sv!', sv.type, sv.id)
free_server(sv)

rpcCb = pc.RPCCB(rpc_cb)
pc.lib.SetRPCCallback(rpcCb)

rpc_msg = cluster_pb.RPCMsg()
rpc_msg.Msg = "hello from python"

ret = send_rpc("connector.testremote.test", rpc_msg, cluster_pb.RPCRes)
print("got rpc res {}".format(ret))
input('Press enter to close program')
pc.lib.Shutdown()
