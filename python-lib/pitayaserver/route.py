""" this module constains pitaya route related logic """


class Route(object):
    """ class for dealing with pitaya routes"""

    def __init__(self, server_type, service, method):
        self.server_type = server_type
        self.service = service
        self.method = method

    def __repr__(self):
        return "%s.%s" % (self.service, self.method)

    def str(self):
        """ returns str representation of route """
        return self.__repr__()

    @classmethod
    def from_str(cls, route_str):
        """ creates a new route from a string """
        route_splitted = route_str.split(".")
        if len(route_splitted) < 3:
            raise Exception("invalid route %s" % route_str)
        instance = cls(route_splitted[0], route_splitted[1], route_splitted[2])
        return instance
