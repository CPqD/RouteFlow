from rflib.defs import *
from TLV import *
from bson.binary import Binary

# Action Type Variables ('Enum')
RFAT_OUTPUT = 1         # Output port
RFAT_SET_ETH_SRC = 2    # Ethernet source address
RFAT_SET_ETH_DST = 3    # Ethernet destination address
RFAT_PUSH_MPLS = 4      # Push MPLS label
RFAT_POP_MPLS = 5       # Pop MPLS label
RFAT_SWAP_MPLS = 6      # Swap MPLS label
# MSB = 1; Indicates optional feature.
RFAT_DROP = 254         # Drop packet (Unimplemented)
RFAT_SFLOW = 255        # Generate SFlow messages (Unimplemented)

typeStrings = {
            RFAT_OUTPUT : "RFAT_OUTPUT",
            RFAT_SET_ETH_SRC : "RFAT_SET_ETH_SRC",
            RFAT_SET_ETH_DST : "RFAT_SET_ETH_DST",
            RFAT_PUSH_MPLS : "RFAT_PUSH_MPLS",
            RFAT_POP_MPLS : "RFAT_POP_MPLS",
            RFAT_SWAP_MPLS : "RFAT_SWAP_MPLS"
        }

class Action(TLV):
    def __init__(self, actionType=None, value=None):
        super(Action, self).__init__(actionType, self.type_to_bin(actionType, value))

    def __str__(self):
        return "%s : %s" % (self.type_to_str(self._type), self.get_value())

    @classmethod
    def OUTPUT(cls, port):
        return cls(RFAT_OUTPUT, port)

    @classmethod
    def SET_ETH_SRC(cls, ethernet_src):
        return cls(RFAT_SET_ETH_SRC, ethernet_src)

    @classmethod
    def SET_ETH_DST(cls, ethernet_dst):
        return cls(RFAT_SET_ETH_DST, ethernet_dst)

    @classmethod
    def PUSH_MPLS(cls, label):
        return cls(RFAT_PUSH_MPLS, label)

    @classmethod
    def POP_MPLS(cls):
        return cls(RFAT_POP_MPLS, None)

    @classmethod
    def SWAP_MPLS(cls, label):
        return cls(RFAT_SWAP_MPLS, label)

    @classmethod
    def DROP(cls):
        return cls(RFAT_DROP, None)

    @classmethod
    def POP_SFLOW(cls):
        return cls(RFAT_POP_SFLOW, None)

    @classmethod
    def CONTROLLER(cls):
        return cls(RFAT_OUTPUT, OFPP_CONTROLLER)

    @classmethod
    def from_dict(cls, dic):
        ac = cls()
        ac._type = dic['type']
        ac._value = dic['value']
        return ac

    @staticmethod
    def type_to_bin(actionType, value):
        if actionType in (RFAT_OUTPUT, RFAT_PUSH_MPLS, RFAT_SWAP_MPLS):
            return int_to_bin(value, 32)
        elif actionType in (RFAT_SET_ETH_SRC, RFAT_SET_ETH_DST):
            return ether_to_bin(value)
        elif actionType in (RFAT_POP_MPLS, RFAT_DROP, RFAT_SFLOW):
            return ''
        else:
            return None

    @staticmethod
    def type_to_str(actionType):
        if actionType in typeStrings:
            return typeStrings[actionType]
        else:
            return str(actionType)

    def get_value(self):
        if self._type in (RFAT_OUTPUT, RFAT_PUSH_MPLS, RFAT_SWAP_MPLS):
            return bin_to_int(self._value)
        elif self._type in (RFAT_SET_ETH_SRC, RFAT_SET_ETH_DST):
            return bin_to_ether(self._value)
        elif self._type in (RFAT_POP_MPLS, RFAT_DROP, RFAT_SFLOW):
            return None
        else:
            return None

    def set_value(self, value):
        self._value = Binary(self.type_to_bin(self._type, value), 0)
