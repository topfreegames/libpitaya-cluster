import go_interop as pc
import uuid as uuid

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

# MUST be called after get_server or else your code will leak
# TODO: can we prevent the user from not calling this?
def free_server(server):
    pc.lib.FreeServer(server)

sv_id = uuid.uuid4().hex

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

input('Press enter to close program')