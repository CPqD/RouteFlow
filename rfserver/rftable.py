import pymongo as mongo
import bson

from rflib.defs import *
from rflib.ipc.MongoIPC import format_address

RFENTRY_IDLE_VM_PORT = 1
RFENTRY_IDLE_DP_PORT = 2
RFENTRY_ASSOCIATED = 3
RFENTRY_ACTIVE = 4

RFENTRY = 0
RFCONFIGENTRY = 1

class MongoTableEntryFactory:
    @staticmethod
    def make(type_):
        if type_ == RFENTRY:
            return RFEntry()
        elif type_ == RFCONFIGENTRY:
            return RFConfigEntry()

class MongoTable:
    def __init__(self, address, name, entry_type):
        self.address = format_address(address)
        self.connection = mongo.Connection(*self.address)
        self.data = self.connection[MONGO_DB_NAME][name]
        self.entry_type = entry_type

    def get_entries(self, **kwargs):
        for (k, v) in kwargs.items():
            kwargs[k] = str(v)
        results = self.data.find(kwargs)
        entries = []
        for result in results:
            entry = MongoTableEntryFactory.make(self.entry_type)
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


class RFTable(MongoTable):
    def __init__(self, address=MONGO_ADDRESS):
        MongoTable.__init__(self, address, RFTABLE_NAME, RFENTRY)

    def get_entry_by_vm_port(self, vm_id, vm_port):
        result = self.get_entries(vm_id=vm_id,
                                  vm_port=vm_port)
        if not result:
            return None
        return result[0]

    def get_entry_by_dp_port(self, ct_id, dp_id, dp_port):
        result = self.get_entries(ct_id=ct_id,
                                  dp_id=dp_id,
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

    def get_dp_entries(self, ct_id, dp_id):
        return self.get_entries(ct_id=ct_id, dp_id=dp_id)

    def is_dp_registered(self, ct_id, dp_id):
        return bool(self.get_dp_entries(ct_id, dp_id))


class RFConfig(MongoTable):
    def __init__(self, ifile, address=MONGO_ADDRESS):
        MongoTable.__init__(self, address, RFCONFIG_NAME, RFCONFIGENTRY)
        # TODO: perform validation of config
        configfile = file(ifile)
        lines = configfile.readlines()[1:]
        entries = [line.strip("\n").split(",") for line in lines]
        for (a, b, c, d, e) in entries:
            self.set_entry(RFConfigEntry(int(a, 16), int(b),
                                         int(c),
                                         int(d, 16), int(e)))

    def get_config_for_vm_port(self, vm_id, vm_port):
        result = self.get_entries(vm_id=vm_id,
                                  vm_port=vm_port)
        if not result:
            return None
        return result[0]

    def get_config_for_dp_port(self, ct_id, dp_id, dp_port):
        result = self.get_entries(ct_id=ct_id,
                                  dp_id=dp_id,
                                  dp_port=dp_port)
        if not result:
            return None
        return result[0]


# Convenience functions for packing/unpacking to a dict for BSON representation
def load_from_dict(src, obj, attr):
    setattr(obj, attr, src[attr])

def pack_into_dict(dest, obj, attr):
    value = getattr(obj, attr)
    dest[attr] = "" if value is None else str(value)


class RFEntry:
    def __init__(self, vm_id=None, vm_port=None, ct_id=None, dp_id=None, 
                 dp_port=None, vs_id=None, vs_port=None, eth_addr=None):
        self.id = None
        self.vm_id = vm_id
        self.vm_port = vm_port
        self.ct_id = ct_id
        self.dp_id = dp_id
        self.dp_port = dp_port
        self.vs_id = vs_id
        self.vs_port = vs_port
        self.eth_addr = eth_addr

    def _is_idle_vm_port(self):
        return (self.vm_id is not None and
                self.vm_port is not None and
                self.ct_id is None and
                self.dp_id is None and
                self.dp_port is None and
                self.vs_id is None and
                self.vs_port is None)

    def _is_idle_dp_port(self):
        return (self.vm_id is None and
                self.vm_port is None and
                self.ct_id is not None and
                self.dp_id is not None and
                self.dp_port is not None and
                self.vs_id is None and
                self.vs_port is None)

    def make_idle(self, type_):
        if type_ == RFENTRY_IDLE_VM_PORT:
            self.ct_id = None
            self.dp_id = None
            self.dp_port = None
            self.vs_id = None
            self.vs_port = None
        elif type_ == RFENTRY_IDLE_DP_PORT:
            self.vm_id = None
            self.vm_port = None
            self.vs_id = None
            self.vs_port = None
            self.eth_addr = None

    def associate(self, id_, port, ct_id=None, eth_addr=None):
        if self._is_idle_vm_port():
            self.ct_id = ct_id
            self.dp_id = id_
            self.dp_port = port
        elif self._is_idle_dp_port():
            self.vm_id = id_
            self.vm_port = port
            self.eth_addr = eth_addr
        else:
            raise ValueError

    def activate(self, vs_id, vs_port):
        self.vs_id = vs_id
        self.vs_port = vs_port

    def get_status(self):
        if self._is_idle_vm_port():
            return RFENTRY_IDLE_VM_PORT
        elif self._is_idle_dp_port():
            return RFENTRY_IDLE_DP_PORT
        elif self.vs_id is None and self.vs_port is None:
            return RFENTRY_ASSOCIATED
        else:
            return RFENTRY_ACTIVE

    def __str__(self):
        return "vm_id: %s\nvm_port: %s\n"\
               "dp_id: %s\ndp_port: %s\n"\
               "vs_id: %s\nvs_port: %s\n"\
               "eth_addr: %s\nct_id: %s\n"\
               "status:%s" % (str(self.vm_id),
                              str(self.vm_port),
                              str(self.dp_id),
                              str(self.dp_port),
                              str(self.vs_id),
                              str(self.vs_port),
                              str(self.eth_addr),
                              str(self.ct_id),
                              str(self.get_status()))

    def from_dict(self, data):
        for k, v in data.items():
            if str(v) is "":
                data[k] = None
            elif k != "_id" and k != "eth_addr": # All our data is int
                data[k] = int(v)
        self.id = data["_id"]
        load_from_dict(data, self, "vm_id")
        load_from_dict(data, self, "vm_port")
        load_from_dict(data, self, "ct_id")
        load_from_dict(data, self, "dp_id")
        load_from_dict(data, self, "dp_port")
        load_from_dict(data, self, "vs_id")
        load_from_dict(data, self, "vs_port")
        load_from_dict(data, self, "eth_addr")

    def to_dict(self):
        data = {}
        if self.id is not None:
            data["_id"] = self.id
        pack_into_dict(data, self, "vm_id")
        pack_into_dict(data, self, "vm_port")
        pack_into_dict(data, self, "ct_id")
        pack_into_dict(data, self, "dp_id")
        pack_into_dict(data, self, "dp_port")
        pack_into_dict(data, self, "vs_id")
        pack_into_dict(data, self, "vs_port")
        pack_into_dict(data, self, "eth_addr")
        return data

class RFConfigEntry:
    def __init__(self, vm_id=None, vm_port=None, ct_id=None, dp_id=None,
                 dp_port=None):
        self.id = None
        self.vm_id = vm_id
        self.vm_port = vm_port
        self.ct_id = ct_id
        self.dp_id = dp_id
        self.dp_port = dp_port

    def __str__(self):
        return "vm_id: %s vm_port: %s "\
               "dp_id: %s dp_port: %s "\
               "ct_id: %s" % (str(self.vm_id), str(self.vm_port),
                              str(self.dp_id), str(self.dp_port),
                              str(self.ct_id))

    def from_dict(self, data):
        for k, v in data.items():
            if str(v) is "":
                data[k] = None
            elif k != "_id": # All our data is int
                data[k] = int(v)
        self.id = data["_id"]
        load_from_dict(data, self, "vm_id")
        load_from_dict(data, self, "vm_port")
        load_from_dict(data, self, "ct_id")
        load_from_dict(data, self, "dp_id")
        load_from_dict(data, self, "dp_port")


    def to_dict(self):
        data = {}
        if self.id is not None:
            data["_id"] = self.id
        pack_into_dict(data, self, "vm_id")
        pack_into_dict(data, self, "vm_port")
        pack_into_dict(data, self, "ct_id")
        pack_into_dict(data, self, "dp_id")
        pack_into_dict(data, self, "dp_port")
        return data
