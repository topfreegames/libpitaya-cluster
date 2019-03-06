import inspect
from google.protobuf import message
import gen.cluster_pb2 as cluster_pb

class RemoteMethod:
    def __init__(self, obj, method, return_type, arg_type):
        self.obj = obj
        self.method = method
        self.return_type = return_type
        self.arg_type = arg_type

class BaseRemote:
    def Name(self):
        return self.__class__

    def IsValidRemote(self):
        m = self.GetRemotesMap()
        return len(m) > 0

    def GetRemotesMap(self):
        res = dict()
        members = inspect.getmembers(self, predicate=inspect.ismethod)
        for m in members:
            fullSpec = inspect.getfullargspec(m[1])
            annotations = fullSpec.annotations
            if len(annotations) > 0:
                # if the return is a subclass of a protobuf message
                if "return" in annotations:
                    if issubclass(annotations['return'], message.Message):
                        if len(fullSpec.args) == 2:
                            # if the class has exactly 2 args, being the second a
                            #subclass of protobuf message
                            if fullSpec.args[1] in annotations:
                                if issubclass(annotations[fullSpec.args[1]], message.Message):
                                    res[m[1].__name__] = RemoteMethod(self, m[1].__func__,
                                                                      annotations['return'],
                                                                      annotations[fullSpec.args[1]])
        return res

# dummy remote class for development purposes
class ExampleRemote(BaseRemote):
    # a valid remote should receive 1 argument that subclasses message.Message
    # and return a class that also subclasses message.Message
    def TestRemote(self, msg:cluster_pb.RPCMsg) -> cluster_pb.RPCMsg:
        invoke_res = cluster_pb.RPCRes()
        invoke_res.Msg = "success! called testremote with msg: %s" % msg.Msg
        invoke_res.Code = 200
        return invoke_res
