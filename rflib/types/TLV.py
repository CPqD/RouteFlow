from socket import *
from binascii import *
from bson.binary import Binary

OPTIONAL_MASK = 1 << 7

class TLV(object):
    def __init__(self, _type=None, _value=None):
        self._type = _type
        if(_type != None):
            self._value = Binary(_value, 0)
        else:
            self._value = _value

    def optional(self):
        if self._type & OPTIONAL_MASK:
            return True;
        return False;

    def get_value_raw(self):
        return self._value

    def to_dict(self):
        return { 'type' : self._type, 'value' : self._value }

def hex_int_extend(num, length):
    return ((length/4 - len(num)) * '0') + num

def int_to_bin(num, length):
    """Converts an integer into a fixed-length integer in network byte order.

    Args:
        num: An integer to serialise.
        length: The number of bits expected in the packed result.

    Returns:
        A structure with the value packed in network byte-order. For example:

        int_to_bin(0xabcd, 16) returns '\xab\xcd'
    """
    hexnum = hex(num)[2:].rstrip('L')
    hexnum = hexnum if len(hexnum) % 2 == 0 else '0' + hexnum
    return a2b_hex(hex_int_extend(hexnum, length))

def bin_to_int(value):
    return int(b2a_hex(value), 16)

def ether_to_bin(ethaddr):
    return a2b_hex(ethaddr.replace(':', ''))

def bin_to_ether(value):
    hexval = b2a_hex(value)
    ethers = '%2s:%2s:%2s:%2s:%2s:%2s' % (hexval[:2], hexval[2:4], hexval[4:6], hexval[6:8], hexval[8:10], hexval[10:])
    return ethers
