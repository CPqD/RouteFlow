from TLV import *
from bson.binary import Binary

RFMT_IPV4 = 1        # Match IPv4 Destination
RFMT_IPV6 = 2        # Match IPv6 Destination
RFMT_ETHERNET = 3    # Match Ethernet Destination
RFMT_MPLS = 4        # Match MPLS label_in
RFMT_ETHERTYPE = 5   # Match Ethernet type
RFMT_NW_PROTO = 6    # Match Network Protocol
RFMT_TP_SRC = 7      # Match Transport Layer Src Port
RFMT_TP_DST = 8      # Match Transport Layer Dest Port
# MSB = 1; Indicates optional feature.
RFMT_IN_PORT = 254   # Match incoming port (Unimplemented)
RFMT_VLAN = 255      # Match incoming VLAN (Unimplemented)

typeStrings = {
            RFMT_IPV4 : "RFMT_IPV4",
            RFMT_IPV6 : "RFMT_IPV6",
            RFMT_ETHERNET : "RFMT_ETHERNET",
            RFMT_MPLS : "RFMT_MPLS",
            RFMT_ETHERTYPE : "RFMT_ETHERTYPE",
            RFMT_NW_PROTO : "RFMT_NW_PROTO",
            RFMT_TP_SRC : "RFMT_TP_SRC",
            RFMT_TP_DST : "RFMT_TP_DST"
        }

class Match(TLV):
    def __init__(self, matchType=None, value=None):
        super(Match, self).__init__(matchType, self.type_to_bin(matchType, value))

    def __str__(self):
        return "%s : %s" % (self.type_to_str(self._type), self.get_value())

    @classmethod
    def IPV4(cls, address, netmask):
        return cls(RFMT_IPV4, (address, netmask))

    @classmethod
    def IPV6(cls, address, netmask):
        return cls(RFMT_IPV6, (address, netmask))

    @classmethod
    def ETHERNET(cls, ethernet_dst):
        return cls(RFMT_ETHERNET, ethernet_dst)

    @classmethod
    def MPLS(cls, label):
        return cls(RFMT_MPLS, label)

    @classmethod
    def IN_PORT(cls, port):
        return cls(RFMT_IN_PORT, port)

    @classmethod
    def VLAN(cls, tag):
        return cls(RFMT_VLAN, tag)

    @classmethod
    def ETHERTYPE(cls, ethertype):
        return cls(RFMT_ETHERTYPE, ethertype)

    @classmethod
    def NW_PROTO(cls, nwproto):
        return cls(RFMT_NW_PROTO, nwproto)

    @classmethod
    def TP_SRC(cls, port):
        return cls(RFMT_TP_SRC, port)

    @classmethod
    def TP_DST(cls, port):
        return cls(RFMT_TP_DST, port)

    @classmethod
    def from_dict(cls, dic):
        ma = cls()
        ma._type = dic['type']
        ma._value = dic['value']
        return ma

    @staticmethod
    def type_to_bin(matchType, value):
        if matchType == RFMT_IPV4:
            return inet_pton(AF_INET, value[0]) + inet_pton(AF_INET, value[1])
        elif matchType == RFMT_IPV6:
            return inet_pton(AF_INET6, value[0]) + inet_pton(AF_INET6, value[1])
        elif matchType == RFMT_ETHERNET:
            return ether_to_bin(value)
        elif matchType in (RFMT_MPLS, RFMT_IN_PORT):
            return int_to_bin(value, 32)
        elif matchType in (RFMT_VLAN, RFMT_ETHERTYPE, RFMT_TP_SRC, RFMT_TP_DST):
            return int_to_bin(value, 16)
        elif matchType == RFMT_NW_PROTO:
            return int_to_bin(value, 8)
        else:
            return None

    @staticmethod
    def type_to_str(matchType):
        if matchType in typeStrings:
            return typeStrings[matchType]
        else:
            return str(matchType)

    def get_value(self):
        if self._type == RFMT_IPV4:
            return (inet_ntop(AF_INET, self._value[:4]), inet_ntop(AF_INET, self._value[4:]))
        elif self._type == RFMT_IPV6:
            return (inet_ntop(AF_INET6, self._value[:16]), inet_ntop(AF_INET6, self._value[16:]))
        elif self._type == RFMT_ETHERNET:
            return bin_to_ether(self._value)
        elif self._type in (RFMT_MPLS, RFMT_IN_PORT, RFMT_VLAN, RFMT_ETHERTYPE,
                            RFMT_NW_PROTO, RFMT_TP_SRC, RFMT_TP_DST):
            return bin_to_int(self._value)
        else:
            return None

    def set_value(value):
        _value = Binary(self.type_to_bin(self._type, value), 0)
