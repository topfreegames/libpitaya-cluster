""" this module contains pitaya's remotes related logic """

import inspect
from google.protobuf import message


class RemoteMethod(object):
    def __init__(self, obj, method, return_type, arg_type):
        self.obj = obj
        self.method = method
        self.return_type = return_type
        self.arg_type = arg_type


class BaseRemote(object):
    """ remotes that you want to register must inherit from BaseRemote """

    def name(self):
        """ returns the name of the class """
        return self.__class__

    def is_valid_remote(self):
        """ if the class has 1 or more valid remote methods, it's valid """
        m = self.get_remotes_map()
        return len(m) > 0

    def get_remotes_map(self):
        """ this method returns the valid remotes present in the class """
        res = dict()
        members = inspect.getmembers(self, predicate=inspect.ismethod)
        for m in members:
            full_spec = inspect.getfullargspec(m[1])
            annotations = full_spec.annotations
            if annotations:
                # if the return is a subclass of a protobuf message
                if "return" in annotations:
                    if issubclass(annotations['return'], message.Message):
                        if len(full_spec.args) == 2:
                            # if the class has exactly 2 args, being the second a
                            # subclass of protobuf message
                            if full_spec.args[1] in annotations:
                                if issubclass(annotations[full_spec.args[1]],
                                              message.Message):
                                    res[m[1].__name__] = RemoteMethod(
                                        self, m[1].__func__,
                                        annotations['return'],
                                        annotations[full_spec.args[1]])
        return res
