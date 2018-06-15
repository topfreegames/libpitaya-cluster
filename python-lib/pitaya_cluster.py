import go_interop as pc
import uuid as uuid

res = pc.lib.Init(
    pc.sd_config(pc.py_to_c_str_array(["127.0.0.1:2379"]), 1, 30, "pitaya/", 30, True, 60),
    pc.nats_rpc_client_config("127.0.0.1:4222", 10, 5000),
    pc.nats_rpc_server_config("127.0.0.1:4222", 10, 75, 10),
    pc.server(uuid.uuid4().hex, "python", "{}", False)
)

if res:
    print("successfully initialized pitaya!")