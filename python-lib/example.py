from uuid import uuid4
from time import sleep
from remote import ExampleRemote
import gen.cluster_pb2 as cluster_pb
import pitaya_cluster as pc
import c_interop as c


def run():
    sv_id = uuid4().hex
    pc.init(
        c.SdConfig(b'http://127.0.0.1:4001', b'pitaya/', 30,
                   True, True, True, 60, c.LOGLEVEL_DEBUG),
        c.NatsConfig(b'127.0.0.1:4222', 5, 5000, 5, 100),
        c.Server(sv_id.encode('utf-8'), b'python', b'{}', b'localhost', False),
    )
    print('successfully initialized pitaya!')
    sv = pc.get_server_by_id(sv_id)
    print('got sv!', sv.type, sv.id)
    # TODO try to use destructor
    # free_server(sv)

    rpc_msg = cluster_pb.RPCMsg()
    rpc_msg.Msg = "hello from python"

    # wait for server to be registered
    sleep(1)

    ret = pc.send_rpc("python.exampleRemote.testRemote",
                      rpc_msg, cluster_pb.RPCRes)
    print("got rpc res {}".format(ret))

    input('Press enter to exit')
    pc.shutdown()


b = ExampleRemote()
pc.register_remote(b, "exampleRemote", pc.default_name_func)

run()
