import rflib.ipc.IPC as IPC
from rflib.ipc.RFProtocol import *

class RFProtocolFactory(IPC.IPCMessageFactory):
    def build_for_type(self, type_):
        if type_ == PORT_REGISTER:
            return PortRegister()
        if type_ == PORT_CONFIG:
            return PortConfig()
        if type_ == DATAPATH_PORT_REGISTER:
            return DatapathPortRegister()
        if type_ == DATAPATH_DOWN:
            return DatapathDown()
        if type_ == VIRTUAL_PLANE_MAP:
            return VirtualPlaneMap()
        if type_ == DATA_PLANE_MAP:
            return DataPlaneMap()
        if type_ == ROUTE_MOD:
            return RouteMod()
