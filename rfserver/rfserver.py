#!/usr/bin/env python
#-*- coding:utf-8 -*-

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
        self.config = [RFConfigEntry(str(a), int(b), str(c), int(d)) for (a, b, c, d) in entries]
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
    def __init__(self):
        self.data = {}
        self.index = 0

    # Basic methods
    def get_entries(self, **kwargs):
        result = []
        for entry in self.data.values():
            match = True
            for (k, v) in kwargs.items():
                if getattr(entry, k) != v:
                    match = False
                    break
            if match:
                result.append(entry)
        return result

    def set_entry(self, entry):
        # TODO: enforce (*_id, *_port) uniqueness restriction
        if entry.id is None:
            entry.id = self.index
            self.index += 1
        self.data[entry.id] = entry

    def remove_entry(self, entry):
        del self.data[entry.id]

    def clear(self):
        self.data = {}
    
    def __str__(self):
        s = ""
        for value in self.data.values():
            s += str(value) + "\n\n"
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

    def get_associated_entry(self, vm_id, vm_port, dp_id, dp_port):
        result = self.get_entries(vm_id=vm_id, vm_port=vm_port,
                                  dp_id=dp_id, dp_port=dp_port)
        if not result:
            return None
        return result[0]

    def get_dp_entries(self, dp_id):
        return self.get_entries(dp_id=dp_id)
                
    def is_dp_registered(self, dp_id):
        return bool(self.get_dp_entries(dp_id))


              
                
REGISTER_IDLE = 0
REGISTER_ASSOCIATED = 1
class RFServer:
    def __init__(self, configfile):
        self.rftable = RFTable()
        self.config = RFConfig(configfile)

    def reset_vm_port(self, vm_id, vm_port):
        pass
        
    def config_vm_port(self, vm_id, vm_port):
        pass
    
    def config_dp(self, dp_id):
        pass

    def register_vm_port(self, vm_id, vm_port):
        action = None
        config_entry = self.config.get_config_for_vm_port(vm_id, vm_port)
        if config_entry is None:
            # Register idle VM awaiting for configuration
            action = REGISTER_IDLE
        else:
            entry = self.rftable.get_entry_by_dp_port(config_entry.dp_id, config_entry.dp_port)
            # If there's no entry, we have no DP, register VM as idle
            if entry is None:
                action = REGISTER_IDLE
            # If there's an idle DP entry matching configuration, associate
            elif entry.get_status() == RFENTRY_IDLE_DP_PORT:
                action = REGISTER_ASSOCIATED

        # Apply action
        if action == REGISTER_IDLE:
            self.rftable.set_entry(RFEntry(vm_id=vm_id, vm_port=vm_port))
        elif action == REGISTER_ASSOCIATED:
            entry.associate(vm_id, vm_port)
            self.rftable.set_entry(entry)
            self.config_vm_port(vm_id, vm_port)

    def register_dp_port(self, dp_id, dp_port):
        if not self.rftable.is_dp_registered(dp_id):
            self.config_dp(dp_id)
            
        action = None
        config_entry = self.config.get_config_for_dp_port(dp_id, dp_port)
        if config_entry is None:
            # Register idle DP awaiting for configuration
            action = REGISTER_IDLE
        else:
            entry = self.rftable.get_entry_by_vm_port(config_entry.vm_id, config_entry.vm_port)
            # If there's no entry, we have no DP, register VM as idle
            if entry is None:
                action = REGISTER_IDLE
            # If there's an idle VM entry matching configuration, associate
            elif entry.get_status() == RFENTRY_IDLE_VM_PORT:
                action = REGISTER_ASSOCIATED

        # Apply action
        if action == REGISTER_IDLE:
            self.rftable.set_entry(RFEntry(dp_id=dp_id, dp_port=dp_port))
        elif action == REGISTER_ASSOCIATED:
            entry.associate(dp_id, dp_port)
            self.rftable.set_entry(entry)
            self.config_vm_port(entry.vm_id, entry.vm_port)

    def map_port(self, vm_id, vm_port, dp_id, dp_port, vs_id, vs_port):
        entry = self.rftable.get_associated_entry(vm_id, vm_port, dp_id, dp_port)
        if entry is not None:
            # If the association is valid, activate it
            entry.activate(vs_id, vs_port)
            self.rftable.set_entry(entry)
    
    def set_dp_port_down(self, dp_id, dp_port):
        entry = self.rftable.get_entry_by_dp_port(dp_id, dp_port)
        if entry is not None:
            # If the DP port is registered, delete it and leave only the 
            # associated VM port. Reset this VM port so it can be reused.
            vm_id, vm_port = entry.vm_id, entry.vm_port
            entry.make_idle(RFENTRY_IDLE_VM_PORT)
            self.rftable.set_entry(entry)
            self.reset_vm_port(vm_id, vm_port)
    
    def set_dp_down(self, dp_id):
        for entry in self.rftable.get_dp_entries(dp_id):
            # For every port registered in that datapath, put it down
            self.set_dp_port_down(entry.dp_id, entry.dp_port)
        
    def __str__(self):
        s = "RFConfig\n" + str(self.config) + "\n"
        s += "RFTable\n" + str(self.rftable)
        return s
          
s = RFServer("rfconfig.csv")
s.register_vm_port("2192192192192192", 2)
s.register_dp_port("7", 2)
s.register_vm_port("2192192192192192", 3)
s.register_dp_port("1231231231231231", 4)
s.map_port("2192192192192192", 2, "7", 2, "42", 43)

s.set_dp_down("7")
print str(s)
