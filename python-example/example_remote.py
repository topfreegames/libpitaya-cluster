import pitayaserver as pc
from gen import cluster_pb2 as cluster_pb


class ExampleRemote(pc.BaseRemote):
    """ example remote for testing purposes """

    # a valid remote should receive 1 argument that subclasses message.Message
    # and return a class that also subclasses message.Message

    def TestRemote(self, msg: cluster_pb.RPCMsg) -> cluster_pb.RPCMsg:
        invoke_res = cluster_pb.RPCRes()
        invoke_res.Msg = "success! called testremote with msg: %s" % msg.Msg
        invoke_res.Code = 200
        return invoke_res
