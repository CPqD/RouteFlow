import pymongo as mongo
import bson

from rflib.defs import *
from rflib.ipc.MongoIPC import format_address

RFENTRY_IDLE_VM_PORT = 1
RFENTRY_IDLE_DP_PORT = 2
RFENTRY_ASSOCIATED = 3
RFENTRY_ACTIVE = 4
RFENTRY_STATUS_NAMES = {
    RFENTRY_IDLE_VM_PORT: "Idle VM port",
    RFENTRY_IDLE_DP_PORT: "Idle DP port",
    RFENTRY_ASSOCIATED: "Associated VM-DP ports",
    RFENTRY_ACTIVE: "Active VM-DP ports",
}

class RFEntry:
    def __init__(self, vm_id=None, vm_port=None, dp_id=None, dp_port=None, vs_id=None, vs_port=None):
        self.id = None
        self.vm_id = vm_id
        self.vm_port = vm_port
        self.dp_id = dp_id
        self.dp_port = dp_port
        self.vs_id = vs_id
        self.vs_port = vs_port
        
    def is_idle_vm_port(self):
        return (self.vm_id is not None and
                self.vm_port is not None and
                self.dp_id is None and
                self.dp_port is None and
                self.vs_id is None and
                self.vs_port is None)

    def is_idle_dp_port(self):
        return (self.vm_id is None and
                self.vm_port is None and
                self.dp_id is not None and
                self.dp_port is not None and
                self.vs_id is None and
                self.vs_port is None)
    
    def make_idle(self, type_):
        if type_ == RFENTRY_IDLE_VM_PORT:
            self.dp_id = None
            self.dp_port = None
            self.vs_id = None
            self.vs_port = None
        elif type_ == RFENTRY_IDLE_DP_PORT:
            self.vm_id = None
            self.vm_port = None
            self.vs_id = None
            self.vs_port = None
                        
    def associate(self, id_, port):
        if self.is_idle_vm_port():
            self.dp_id = id_
            self.dp_port = port
        elif self.is_idle_dp_port():
            self.vm_id = id_
            self.vm_port = port
        else:
            raise ValueError

    def activate(self, vs_id, vs_port):
        self.vs_id = vs_id
        self.vs_port = vs_port

    def get_status(self):
        if self.is_idle_vm_port():
            return RFENTRY_IDLE_VM_PORT
        elif self.is_idle_dp_port():
            return RFENTRY_IDLE_DP_PORT
        elif self.vs_id is None and self.vs_port is None:
            return RFENTRY_ASSOCIATED
        else:
            return RFENTRY_ACTIVE


    def __str__(self):
        return "vm_id: %s\nvm_port: %s\n"\
               "dp_id: %s\ndp_port: %s\n"\
               "vs_id: %s\nvs_port: %s\n"\
               "status:%s" % (str(self.vm_id), 
                              str(self.vm_port), 
                              str(self.dp_id), 
                              str(self.dp_port),
                              str(self.vs_id), 
                              str(self.vs_port), 
                              RFENTRY_STATUS_NAMES[self.get_status()])
    
    # TODO: use bson? move to rftable?
    def from_dict(self, data):
        for k, v in data.items():
            if str(v) is "":
                data[k] = None
        def load(src, attr, obj):
            setattr(obj, attr, src[attr])
            
        self.id = data["_id"]
        load(data, "vm_id", self)
        load(data, "vm_port", self)
        load(data, "dp_id", self)
        load(data, "dp_port", self)
        load(data, "vs_id", self)
        load(data, "vs_port", self)
        
    def to_dict(self):
        def pack(dest, attr, obj):
            value = getattr(obj, attr)
            dest[attr] = "" if value is None else str(value)
        data = {}
        if self.id is not None:
            data["_id"] = self.id
        pack(data, "vm_id", self)
        pack(data, "vm_port", self)
        pack(data, "dp_id", self)
        pack(data, "dp_port", self)
        pack(data, "vs_id", self)
        pack(data, "vs_port", self)
        return data


class RFConfigEntry:
    def __init__(self, vm_id=None, vm_port=None, dp_id=None, dp_port=None):
        self.vm_id = vm_id
        self.vm_port = vm_port
        self.dp_id = dp_id
        self.dp_port = dp_port

    def __str__(self):
        return "vm_id: %s vm_port: %s "\
               "dp_id: %s dp_port: %s" % (str(self.vm_id), 
                                             str(self.vm_port), 
                                             str(self.dp_id), 
                                             str(self.dp_port))
                              
class RFConfig:
    def __init__(self, ifile):
        configfile = file(ifile)
        entries = [line.strip("\n").split(",") for line in configfile.readlines()[1:]]
        self.config = [RFConfigEntry(int(a), int(b), int(c), int(d)) for (a, b, c, d) in entries]
        # TODO: perform validation of config

    def get_config_for_vm_port(self, vm_id, vm_port):
        for entry in self.config:
            if entry.vm_id == vm_id and entry.vm_port == vm_port:
                return entry

    def get_config_for_dp_port(self, dp_id, dp_port):
        for entry in self.config:
            if entry.dp_id == dp_id and entry.dp_port == dp_port:
                return entry

    def __str__(self):
        return "\n".join([str(e) for e in self.config]) + "\n"
        
            
class RFTable:
    def __init__(self, address=MONGO_ADDRESS):
        # self.data = {}
        # self.index = 0
        self.address = format_address(address)
        self.connection = mongo.Connection(*self.address)
        self.data = self.connection[MONGO_DB_NAME][RF_TABLE_NAME]
        
    # Basic methods
    def get_entries(self, **kwargs):
        for (k, v) in kwargs.items():
            kwargs[k] = str(v)
        results = self.data.find(kwargs)
        entries = []
        for result in results:
            entry = RFEntry()
            entry.from_dict(result)
            entries.append(entry)
        return entries

    def set_entry(self, entry):
        # TODO: enforce (*_id, *_port) uniqueness restriction
        entry.id = self.data.save(entry.to_dict())
        
    def remove_entry(self, entry):
        self.data.remove(entry.id)

    def clear(self):
        self.data.remove()
    
    def __str__(self):
        s = ""
        for entry in self.get_entries():
            s += str(entry) + "\n\n"
        return s.strip("\n")
        
    # Higher level methods
    def get_entry_by_vm_port(self, vm_id, vm_port):
        result = self.get_entries(vm_id=vm_id,
                                  vm_port=vm_port)
        if not result:
            return None
        return result[0]

    def get_entry_by_dp_port(self, dp_id, dp_port):
        result = self.get_entries(dp_id=dp_id, 
                                  dp_port=dp_port)
        if not result:
            return None
        return result[0]

    def get_entry_by_vs_port(self, vs_id, vs_port):
        result = self.get_entries(vs_id=vs_id, 
                                  vs_port=vs_port)
        if not result:
            return None
        return result[0]
        
    def get_dp_entries(self, dp_id):
        return self.get_entries(dp_id=dp_id)
                
    def is_dp_registered(self, dp_id):
        return bool(self.get_dp_entries(dp_id))

