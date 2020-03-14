from uuid import uuid4
from time import sleep
import pitayaserver as pc
from gen import cluster_pb2 as cluster_pb
from example_remote import ExampleRemote


def send_test_rpc(route):
    rpc_msg = cluster_pb.RPCMsg()
    rpc_msg.Msg = "hello from python"
    print("sending test rpc with route %s" % route)
    ret = pc.send_rpc(route,
                      rpc_msg, cluster_pb.RPCRes)
    print("got rpc res {}".format(ret))


def run():
    sv_id = uuid4().hex
    pc.initialize_pitaya(
        pc.SdConfig(b'http://127.0.0.1:4001', b'pitaya/', b'[]', 30,
                    True, True, True, 30, pc.LogLevel.DEBUG.value),
        pc.NatsConfig(b'127.0.0.1:4222', 100, 5000, 500, 1000, 5, 100),
        pc.Server(sv_id.encode('utf-8'), b'python',
                  b'{}', b'localhost', False),
        pc.LogLevel.DEBUG.value,
    )
    print('successfully initialized pitaya!')

    # wait for server to be registered
    sleep(1)

    # send a test RPC to connector
    try:
        send_test_rpc("connector.testremote.test")
    except Exception as e:
        print('error sending rpc to connector! ' + str(e))

    # register the example remote
    r = ExampleRemote()
    pc.register_remote(r, "exampleRemote", pc.default_name_func)

    try:
        # send a test RPC to self
        send_test_rpc("python.exampleRemote.testRemote")
    except Exception as e:
        print('error sending rpc to python ' + str(e))
    
    input('Press enter to exit')
    pc.shutdown()


run()
