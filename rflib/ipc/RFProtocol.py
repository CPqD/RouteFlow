import MongoIPC

PORT_REGISTER = 0
PORT_CONFIG = 1
DATAPATH_CONFIG = 2
ROUTE_INFO = 3
FLOW_MOD = 4
DATAPATH_PORT_REGISTER = 5
DATAPATH_DOWN = 6
PORT_MAP = 7

class PortRegister(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, PORT_REGISTER, **kwargs)

    def str(self):
        string = "PortRegister\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class PortConfig(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, PORT_CONFIG, **kwargs)

    def str(self):
        string = "PortConfig\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
        
class DatapathConfig(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_CONFIG, **kwargs)

    def str(self):
        string = "DatapathConfig\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class RouteInfo(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, ROUTE_INFO, **kwargs)

    def str(self):
        string = "RouteInfo\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
        
class FlowMod(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, FLOW_MOD, **kwargs)

    def str(self):
        string = "FlowMod\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
        
class DatapathPortRegister(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_PORT_REGISTER, **kwargs)

    def str(self):
        string = "DatapathPortRegister\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
        
class DatapathDown(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_DOWN, **kwargs)

    def str(self):
        string = "DatapathDown\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
        
class PortMap(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, PORT_MAP, **kwargs)

    def str(self):
        string = "PortMap\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
