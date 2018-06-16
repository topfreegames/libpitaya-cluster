import go_interop as pc
import uuid as uuid

def init(
    sd_config:pc.SdConfig,
    nats_rpc_client_config:pc.NatsRpcClientConfig,
    nats_rpc_server_config:pc.NatsRpcServerConfig,
    server:pc.Server):
    return pc.lib.Init(sd_config, nats_rpc_client_config, nats_rpc_server_config, server)

def get_server(server_id:str):
    res = pc.lib.GetServer(pc.GoString(server_id.encode('utf-8'), len(server_id)))
    if res.contents.success == False:
        raise Exception('failed to get server {}'.format(server_id))
    return res.contents.server.contents

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
print('got sv!', sv.id)

input('Press enter to close program')