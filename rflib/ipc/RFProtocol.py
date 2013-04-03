import bson
import pymongo as mongo

from MongoIPC import MongoIPCMessage

PORT_REGISTER = 0
PORT_CONFIG = 1
DATAPATH_CONFIG = 2
ROUTE_INFO = 3
FLOW_MOD = 4
DATAPATH_PORT_REGISTER = 5
DATAPATH_DOWN = 6
VIRTUAL_PLANE_MAP = 7
DATA_PLANE_MAP = 8
ROUTE_MOD = 9

class PortRegister(MongoIPCMessage):
    def __init__(self, vm_id=None, vm_port=None):
        self.set_vm_id(vm_id)
        self.set_vm_port(vm_port)

    def get_type(self):
        return PORT_REGISTER

    def get_vm_id(self):
        return self.vm_id

    def set_vm_id(self, vm_id):
        vm_id = 0 if vm_id is None else vm_id
        try:
            self.vm_id = int(vm_id)
        except:
            self.vm_id = 0

    def get_vm_port(self):
        return self.vm_port

    def set_vm_port(self, vm_port):
        vm_port = 0 if vm_port is None else vm_port
        try:
            self.vm_port = int(vm_port)
        except:
            self.vm_port = 0

    def from_dict(self, data):
        self.set_vm_id(data["vm_id"])
        self.set_vm_port(data["vm_port"])

    def to_dict(self):
        data = {}
        data["vm_id"] = str(self.get_vm_id())
        data["vm_port"] = str(self.get_vm_port())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "PortRegister\n"
        s += "  vm_id: " + str(self.get_vm_id()) + "\n"
        s += "  vm_port: " + str(self.get_vm_port()) + "\n"
        return s

class PortConfig(MongoIPCMessage):
    def __init__(self, vm_id=None, vm_port=None, operation_id=None):
        self.set_vm_id(vm_id)
        self.set_vm_port(vm_port)
        self.set_operation_id(operation_id)

    def get_type(self):
        return PORT_CONFIG

    def get_vm_id(self):
        return self.vm_id

    def set_vm_id(self, vm_id):
        vm_id = 0 if vm_id is None else vm_id
        try:
            self.vm_id = int(vm_id)
        except:
            self.vm_id = 0

    def get_vm_port(self):
        return self.vm_port

    def set_vm_port(self, vm_port):
        vm_port = 0 if vm_port is None else vm_port
        try:
            self.vm_port = int(vm_port)
        except:
            self.vm_port = 0

    def get_operation_id(self):
        return self.operation_id

    def set_operation_id(self, operation_id):
        operation_id = 0 if operation_id is None else operation_id
        try:
            self.operation_id = int(operation_id)
        except:
            self.operation_id = 0

    def from_dict(self, data):
        self.set_vm_id(data["vm_id"])
        self.set_vm_port(data["vm_port"])
        self.set_operation_id(data["operation_id"])

    def to_dict(self):
        data = {}
        data["vm_id"] = str(self.get_vm_id())
        data["vm_port"] = str(self.get_vm_port())
        data["operation_id"] = str(self.get_operation_id())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "PortConfig\n"
        s += "  vm_id: " + str(self.get_vm_id()) + "\n"
        s += "  vm_port: " + str(self.get_vm_port()) + "\n"
        s += "  operation_id: " + str(self.get_operation_id()) + "\n"
        return s

class DatapathConfig(MongoIPCMessage):
    def __init__(self, ct_id=None, dp_id=None, operation_id=None):
        self.set_ct_id(ct_id)
        self.set_dp_id(dp_id)
        self.set_operation_id(operation_id)

    def get_type(self):
        return DATAPATH_CONFIG

    def get_ct_id(self):
        return self.ct_id

    def set_ct_id(self, ct_id):
        ct_id = 0 if ct_id is None else ct_id
        try:
            self.ct_id = int(ct_id)
        except:
            self.ct_id = 0

    def get_dp_id(self):
        return self.dp_id

    def set_dp_id(self, dp_id):
        dp_id = 0 if dp_id is None else dp_id
        try:
            self.dp_id = int(dp_id)
        except:
            self.dp_id = 0

    def get_operation_id(self):
        return self.operation_id

    def set_operation_id(self, operation_id):
        operation_id = 0 if operation_id is None else operation_id
        try:
            self.operation_id = int(operation_id)
        except:
            self.operation_id = 0

    def from_dict(self, data):
        self.set_ct_id(data["ct_id"])
        self.set_dp_id(data["dp_id"])
        self.set_operation_id(data["operation_id"])

    def to_dict(self):
        data = {}
        data["ct_id"] = str(self.get_ct_id())
        data["dp_id"] = str(self.get_dp_id())
        data["operation_id"] = str(self.get_operation_id())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "DatapathConfig\n"
        s += "  ct_id: " + str(self.get_ct_id()) + "\n"
        s += "  dp_id: " + str(self.get_dp_id()) + "\n"
        s += "  operation_id: " + str(self.get_operation_id()) + "\n"
        return s

class RouteInfo(MongoIPCMessage):
    def __init__(self, vm_id=None, vm_port=None, address=None, netmask=None, dst_port=None, src_hwaddress=None, dst_hwaddress=None, is_removal=None):
        self.set_vm_id(vm_id)
        self.set_vm_port(vm_port)
        self.set_address(address)
        self.set_netmask(netmask)
        self.set_dst_port(dst_port)
        self.set_src_hwaddress(src_hwaddress)
        self.set_dst_hwaddress(dst_hwaddress)
        self.set_is_removal(is_removal)

    def get_type(self):
        return ROUTE_INFO

    def get_vm_id(self):
        return self.vm_id

    def set_vm_id(self, vm_id):
        vm_id = 0 if vm_id is None else vm_id
        try:
            self.vm_id = int(vm_id)
        except:
            self.vm_id = 0

    def get_vm_port(self):
        return self.vm_port

    def set_vm_port(self, vm_port):
        vm_port = 0 if vm_port is None else vm_port
        try:
            self.vm_port = int(vm_port)
        except:
            self.vm_port = 0

    def get_address(self):
        return self.address

    def set_address(self, address):
        address = "" if address is None else address
        try:
            self.address = str(address)
        except:
            self.address = ""

    def get_netmask(self):
        return self.netmask

    def set_netmask(self, netmask):
        netmask = "" if netmask is None else netmask
        try:
            self.netmask = str(netmask)
        except:
            self.netmask = ""

    def get_dst_port(self):
        return self.dst_port

    def set_dst_port(self, dst_port):
        dst_port = 0 if dst_port is None else dst_port
        try:
            self.dst_port = int(dst_port)
        except:
            self.dst_port = 0

    def get_src_hwaddress(self):
        return self.src_hwaddress

    def set_src_hwaddress(self, src_hwaddress):
        src_hwaddress = "" if src_hwaddress is None else src_hwaddress
        try:
            self.src_hwaddress = str(src_hwaddress)
        except:
            self.src_hwaddress = ""

    def get_dst_hwaddress(self):
        return self.dst_hwaddress

    def set_dst_hwaddress(self, dst_hwaddress):
        dst_hwaddress = "" if dst_hwaddress is None else dst_hwaddress
        try:
            self.dst_hwaddress = str(dst_hwaddress)
        except:
            self.dst_hwaddress = ""

    def get_is_removal(self):
        return self.is_removal

    def set_is_removal(self, is_removal):
        is_removal = False if is_removal is None else is_removal
        try:
            self.is_removal = bool(is_removal)
        except:
            self.is_removal = False

    def from_dict(self, data):
        self.set_vm_id(data["vm_id"])
        self.set_vm_port(data["vm_port"])
        self.set_address(data["address"])
        self.set_netmask(data["netmask"])
        self.set_dst_port(data["dst_port"])
        self.set_src_hwaddress(data["src_hwaddress"])
        self.set_dst_hwaddress(data["dst_hwaddress"])
        self.set_is_removal(data["is_removal"])

    def to_dict(self):
        data = {}
        data["vm_id"] = str(self.get_vm_id())
        data["vm_port"] = str(self.get_vm_port())
        data["address"] = str(self.get_address())
        data["netmask"] = str(self.get_netmask())
        data["dst_port"] = str(self.get_dst_port())
        data["src_hwaddress"] = str(self.get_src_hwaddress())
        data["dst_hwaddress"] = str(self.get_dst_hwaddress())
        data["is_removal"] = bool(self.get_is_removal())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "RouteInfo\n"
        s += "  vm_id: " + str(self.get_vm_id()) + "\n"
        s += "  vm_port: " + str(self.get_vm_port()) + "\n"
        s += "  address: " + str(self.get_address()) + "\n"
        s += "  netmask: " + str(self.get_netmask()) + "\n"
        s += "  dst_port: " + str(self.get_dst_port()) + "\n"
        s += "  src_hwaddress: " + str(self.get_src_hwaddress()) + "\n"
        s += "  dst_hwaddress: " + str(self.get_dst_hwaddress()) + "\n"
        s += "  is_removal: " + str(self.get_is_removal()) + "\n"
        return s

class FlowMod(MongoIPCMessage):
    def __init__(self, ct_id=None, dp_id=None, address=None, netmask=None, dst_port=None, src_hwaddress=None, dst_hwaddress=None, is_removal=None):
        self.set_ct_id(ct_id)
        self.set_dp_id(dp_id)
        self.set_address(address)
        self.set_netmask(netmask)
        self.set_dst_port(dst_port)
        self.set_src_hwaddress(src_hwaddress)
        self.set_dst_hwaddress(dst_hwaddress)
        self.set_is_removal(is_removal)

    def get_type(self):
        return FLOW_MOD

    def get_ct_id(self):
        return self.ct_id

    def set_ct_id(self, ct_id):
        ct_id = 0 if ct_id is None else ct_id
        try:
            self.ct_id = int(ct_id)
        except:
            self.ct_id = 0

    def get_dp_id(self):
        return self.dp_id

    def set_dp_id(self, dp_id):
        dp_id = 0 if dp_id is None else dp_id
        try:
            self.dp_id = int(dp_id)
        except:
            self.dp_id = 0

    def get_address(self):
        return self.address

    def set_address(self, address):
        address = "" if address is None else address
        try:
            self.address = str(address)
        except:
            self.address = ""

    def get_netmask(self):
        return self.netmask

    def set_netmask(self, netmask):
        netmask = "" if netmask is None else netmask
        try:
            self.netmask = str(netmask)
        except:
            self.netmask = ""

    def get_dst_port(self):
        return self.dst_port

    def set_dst_port(self, dst_port):
        dst_port = 0 if dst_port is None else dst_port
        try:
            self.dst_port = int(dst_port)
        except:
            self.dst_port = 0

    def get_src_hwaddress(self):
        return self.src_hwaddress

    def set_src_hwaddress(self, src_hwaddress):
        src_hwaddress = "" if src_hwaddress is None else src_hwaddress
        try:
            self.src_hwaddress = str(src_hwaddress)
        except:
            self.src_hwaddress = ""

    def get_dst_hwaddress(self):
        return self.dst_hwaddress

    def set_dst_hwaddress(self, dst_hwaddress):
        dst_hwaddress = "" if dst_hwaddress is None else dst_hwaddress
        try:
            self.dst_hwaddress = str(dst_hwaddress)
        except:
            self.dst_hwaddress = ""

    def get_is_removal(self):
        return self.is_removal

    def set_is_removal(self, is_removal):
        is_removal = False if is_removal is None else is_removal
        try:
            self.is_removal = bool(is_removal)
        except:
            self.is_removal = False

    def from_dict(self, data):
        self.set_ct_id(data["ct_id"])
        self.set_dp_id(data["dp_id"])
        self.set_address(data["address"])
        self.set_netmask(data["netmask"])
        self.set_dst_port(data["dst_port"])
        self.set_src_hwaddress(data["src_hwaddress"])
        self.set_dst_hwaddress(data["dst_hwaddress"])
        self.set_is_removal(data["is_removal"])

    def to_dict(self):
        data = {}
        data["ct_id"] = str(self.get_ct_id())
        data["dp_id"] = str(self.get_dp_id())
        data["address"] = str(self.get_address())
        data["netmask"] = str(self.get_netmask())
        data["dst_port"] = str(self.get_dst_port())
        data["src_hwaddress"] = str(self.get_src_hwaddress())
        data["dst_hwaddress"] = str(self.get_dst_hwaddress())
        data["is_removal"] = bool(self.get_is_removal())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "FlowMod\n"
        s += "  ct_id: " + str(self.get_ct_id()) + "\n"
        s += "  dp_id: " + str(self.get_dp_id()) + "\n"
        s += "  address: " + str(self.get_address()) + "\n"
        s += "  netmask: " + str(self.get_netmask()) + "\n"
        s += "  dst_port: " + str(self.get_dst_port()) + "\n"
        s += "  src_hwaddress: " + str(self.get_src_hwaddress()) + "\n"
        s += "  dst_hwaddress: " + str(self.get_dst_hwaddress()) + "\n"
        s += "  is_removal: " + str(self.get_is_removal()) + "\n"
        return s

class DatapathPortRegister(MongoIPCMessage):
    def __init__(self, ct_id=None, dp_id=None, dp_port=None):
        self.set_ct_id(ct_id)
        self.set_dp_id(dp_id)
        self.set_dp_port(dp_port)

    def get_type(self):
        return DATAPATH_PORT_REGISTER

    def get_ct_id(self):
        return self.ct_id

    def set_ct_id(self, ct_id):
        ct_id = 0 if ct_id is None else ct_id
        try:
            self.ct_id = int(ct_id)
        except:
            self.ct_id = 0

    def get_dp_id(self):
        return self.dp_id

    def set_dp_id(self, dp_id):
        dp_id = 0 if dp_id is None else dp_id
        try:
            self.dp_id = int(dp_id)
        except:
            self.dp_id = 0

    def get_dp_port(self):
        return self.dp_port

    def set_dp_port(self, dp_port):
        dp_port = 0 if dp_port is None else dp_port
        try:
            self.dp_port = int(dp_port)
        except:
            self.dp_port = 0

    def from_dict(self, data):
        self.set_ct_id(data["ct_id"])
        self.set_dp_id(data["dp_id"])
        self.set_dp_port(data["dp_port"])

    def to_dict(self):
        data = {}
        data["ct_id"] = str(self.get_ct_id())
        data["dp_id"] = str(self.get_dp_id())
        data["dp_port"] = str(self.get_dp_port())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "DatapathPortRegister\n"
        s += "  ct_id: " + str(self.get_ct_id()) + "\n"
        s += "  dp_id: " + str(self.get_dp_id()) + "\n"
        s += "  dp_port: " + str(self.get_dp_port()) + "\n"
        return s

class DatapathDown(MongoIPCMessage):
    def __init__(self, ct_id=None, dp_id=None):
        self.set_ct_id(ct_id)
        self.set_dp_id(dp_id)

    def get_type(self):
        return DATAPATH_DOWN

    def get_ct_id(self):
        return self.ct_id

    def set_ct_id(self, ct_id):
        ct_id = 0 if ct_id is None else ct_id
        try:
            self.ct_id = int(ct_id)
        except:
            self.ct_id = 0

    def get_dp_id(self):
        return self.dp_id

    def set_dp_id(self, dp_id):
        dp_id = 0 if dp_id is None else dp_id
        try:
            self.dp_id = int(dp_id)
        except:
            self.dp_id = 0

    def from_dict(self, data):
        self.set_ct_id(data["ct_id"])
        self.set_dp_id(data["dp_id"])

    def to_dict(self):
        data = {}
        data["ct_id"] = str(self.get_ct_id())
        data["dp_id"] = str(self.get_dp_id())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "DatapathDown\n"
        s += "  ct_id: " + str(self.get_ct_id()) + "\n"
        s += "  dp_id: " + str(self.get_dp_id()) + "\n"
        return s

class VirtualPlaneMap(MongoIPCMessage):
    def __init__(self, vm_id=None, vm_port=None, vs_id=None, vs_port=None):
        self.set_vm_id(vm_id)
        self.set_vm_port(vm_port)
        self.set_vs_id(vs_id)
        self.set_vs_port(vs_port)

    def get_type(self):
        return VIRTUAL_PLANE_MAP

    def get_vm_id(self):
        return self.vm_id

    def set_vm_id(self, vm_id):
        vm_id = 0 if vm_id is None else vm_id
        try:
            self.vm_id = int(vm_id)
        except:
            self.vm_id = 0

    def get_vm_port(self):
        return self.vm_port

    def set_vm_port(self, vm_port):
        vm_port = 0 if vm_port is None else vm_port
        try:
            self.vm_port = int(vm_port)
        except:
            self.vm_port = 0

    def get_vs_id(self):
        return self.vs_id

    def set_vs_id(self, vs_id):
        vs_id = 0 if vs_id is None else vs_id
        try:
            self.vs_id = int(vs_id)
        except:
            self.vs_id = 0

    def get_vs_port(self):
        return self.vs_port

    def set_vs_port(self, vs_port):
        vs_port = 0 if vs_port is None else vs_port
        try:
            self.vs_port = int(vs_port)
        except:
            self.vs_port = 0

    def from_dict(self, data):
        self.set_vm_id(data["vm_id"])
        self.set_vm_port(data["vm_port"])
        self.set_vs_id(data["vs_id"])
        self.set_vs_port(data["vs_port"])

    def to_dict(self):
        data = {}
        data["vm_id"] = str(self.get_vm_id())
        data["vm_port"] = str(self.get_vm_port())
        data["vs_id"] = str(self.get_vs_id())
        data["vs_port"] = str(self.get_vs_port())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "VirtualPlaneMap\n"
        s += "  vm_id: " + str(self.get_vm_id()) + "\n"
        s += "  vm_port: " + str(self.get_vm_port()) + "\n"
        s += "  vs_id: " + str(self.get_vs_id()) + "\n"
        s += "  vs_port: " + str(self.get_vs_port()) + "\n"
        return s

class DataPlaneMap(MongoIPCMessage):
    def __init__(self, ct_id=None, dp_id=None, dp_port=None, vs_id=None, vs_port=None):
        self.set_ct_id(ct_id)
        self.set_dp_id(dp_id)
        self.set_dp_port(dp_port)
        self.set_vs_id(vs_id)
        self.set_vs_port(vs_port)

    def get_type(self):
        return DATA_PLANE_MAP

    def get_ct_id(self):
        return self.ct_id

    def set_ct_id(self, ct_id):
        ct_id = 0 if ct_id is None else ct_id
        try:
            self.ct_id = int(ct_id)
        except:
            self.ct_id = 0

    def get_dp_id(self):
        return self.dp_id

    def set_dp_id(self, dp_id):
        dp_id = 0 if dp_id is None else dp_id
        try:
            self.dp_id = int(dp_id)
        except:
            self.dp_id = 0

    def get_dp_port(self):
        return self.dp_port

    def set_dp_port(self, dp_port):
        dp_port = 0 if dp_port is None else dp_port
        try:
            self.dp_port = int(dp_port)
        except:
            self.dp_port = 0

    def get_vs_id(self):
        return self.vs_id

    def set_vs_id(self, vs_id):
        vs_id = 0 if vs_id is None else vs_id
        try:
            self.vs_id = int(vs_id)
        except:
            self.vs_id = 0

    def get_vs_port(self):
        return self.vs_port

    def set_vs_port(self, vs_port):
        vs_port = 0 if vs_port is None else vs_port
        try:
            self.vs_port = int(vs_port)
        except:
            self.vs_port = 0

    def from_dict(self, data):
        self.set_ct_id(data["ct_id"])
        self.set_dp_id(data["dp_id"])
        self.set_dp_port(data["dp_port"])
        self.set_vs_id(data["vs_id"])
        self.set_vs_port(data["vs_port"])

    def to_dict(self):
        data = {}
        data["ct_id"] = str(self.get_ct_id())
        data["dp_id"] = str(self.get_dp_id())
        data["dp_port"] = str(self.get_dp_port())
        data["vs_id"] = str(self.get_vs_id())
        data["vs_port"] = str(self.get_vs_port())
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "DataPlaneMap\n"
        s += "  ct_id: " + str(self.get_ct_id()) + "\n"
        s += "  dp_id: " + str(self.get_dp_id()) + "\n"
        s += "  dp_port: " + str(self.get_dp_port()) + "\n"
        s += "  vs_id: " + str(self.get_vs_id()) + "\n"
        s += "  vs_port: " + str(self.get_vs_port()) + "\n"
        return s

class RouteMod(MongoIPCMessage):
    def __init__(self, mod=None, id=None, matches=None, actions=None, options=None):
        self.set_mod(mod)
        self.set_id(id)
        self.set_matches(matches)
        self.set_actions(actions)
        self.set_options(options)

    def get_type(self):
        return ROUTE_MOD

    def get_mod(self):
        return self.mod

    def set_mod(self, mod):
        mod = 0 if mod is None else mod
        try:
            self.mod = int(mod)
        except:
            self.mod = 0

    def get_id(self):
        return self.id

    def set_id(self, id):
        id = 0 if id is None else id
        try:
            self.id = int(id)
        except:
            self.id = 0

    def get_matches(self):
        return self.matches

    def set_matches(self, matches):
        matches = list() if matches is None else matches
        try:
            self.matches = list(matches)
        except:
            self.matches = list()

    def add_match(self, match):
        self.matches.append(match.to_dict())

    def get_actions(self):
        return self.actions

    def set_actions(self, actions):
        actions = list() if actions is None else actions
        try:
            self.actions = list(actions)
        except:
            self.actions = list()

    def add_action(self, action):
        self.actions.append(action.to_dict())

    def get_options(self):
        return self.options

    def set_options(self, options):
        options = list() if options is None else options
        try:
            self.options = list(options)
        except:
            self.options = list()

    def add_option(self, option):
        self.options.append(option.to_dict())

    def from_dict(self, data):
        self.set_mod(data["mod"])
        self.set_id(data["id"])
        self.set_matches(data["matches"])
        self.set_actions(data["actions"])
        self.set_options(data["options"])

    def to_dict(self):
        data = {}
        data["mod"] = self.get_mod()
        data["id"] = str(self.get_id())
        data["matches"] = self.get_matches()
        data["actions"] = self.get_actions()
        data["options"] = self.get_options()
        return data

    def from_bson(self, data):
        data = bson.BSON.decode(data)
        self.from_dict(data)

    def to_bson(self):
        return bson.BSON.encode(self.get_dict())

    def __str__(self):
        s = "RouteMod\n"
        s += "  mod: " + str(self.get_mod()) + "\n"
        s += "  id: " + str(self.get_id()) + "\n"
        s += "  matches:\n"
        for match in self.get_matches():
            s += "    " + str(Match.from_dict(match)) + "\n"
        s += "  actions:\n"
        for action in self.get_actions():
            s += "    " + str(Action.from_dict(action)) + "\n"
        s += "  options:\n"
        for option in self.get_options():
            s += "    " + str(Option.from_dict(option)) + "\n"
        return s
