import MongoIPC

VM_REGISTER_REQUEST = 0
VM_REGISTER_RESPONSE = 1
VM_CONFIG = 2
DATAPATH_CONFIG = 3
ROUTE_INFO = 4
FLOW_MOD = 5
DATAPATH_JOIN = 6
DATAPATH_LEAVE = 7
VM_MAP = 8

class DatapathJoin(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_JOIN, **kwargs)

    def str(self):
        string = "DatapathJoin\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class DatapathConfig(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_CONFIG, **kwargs)

    def str(self):
        string = "DatapathConfig\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class FlowMod(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, FLOW_MOD, **kwargs)

    def str(self):
        string = "FlowMod\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class VMMap(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, VM_MAP, **kwargs)

    def str(self):
        string = "VMMap\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string
