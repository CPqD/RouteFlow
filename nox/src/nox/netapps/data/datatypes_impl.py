#
# Copyright 2009 (C) Nicira, Inc.
#

import logging

from nox.lib.core import Component
from nox.netapps.data.pydatatypes import PyDatatypes

lg = logging.getLogger('nox.netapps.data.datatypes')

class Datatypes(Component):

    # Static types for *Info objects, not to be used by other
    # components

    _SWITCH       = -1
    _LOCATION     = -1
    _HOST         = -1
    _HOST_NETID   = -1
    _USER         = -1
    _ADDRESS      = -1
    _GROUP        = -1
    _GROUP_MEMBER = -1

    _MAC          = -1
    _IPV4_CIDR    = -1
    _IPV6         = -1
    _ALIAS        = -1

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.ctypes = PyDatatypes(ctxt)
        self.SWITCH = None
        self.LOCATION = None
        self.HOST = None
        self.HOST_NETID = None
        self.USER = None
        self.ADDRESS = None
        self.GROUP = None
        self.GROUP_MEMBER = None

        self.MAC = None
        self.IPV4_CIDR = None
        self.IPV6 = None
        self.ALIAS = None

    def getInterface(self):
        return str(Datatypes)

    def configure(self, config):
        self.ctypes.configure(config)

    def install(self):
        self.SWITCH = Datatypes._SWITCH = self.ctypes.switch_type()
        self.LOCATION = Datatypes._LOCATION = self.ctypes.location_type()
        self.HOST = Datatypes._HOST = self.ctypes.host_type()
        self.HOST_NETID = Datatypes._HOST_NETID = self.ctypes.host_netid_type()
        self.USER = Datatypes._USER = self.ctypes.user_type()
        self.ADDRESS = Datatypes._ADDRESS = self.ctypes.address_type()
        self.GROUP = Datatypes._GROUP = self.ctypes.group_type()
        self.GROUP_MEMBER = Datatypes._GROUP_MEMBER \
            = self.ctypes.group_member_type()

        self.MAC = Datatypes._MAC = self.ctypes.mac_type()
        self.IPV4_CIDR = Datatypes._IPV4_CIDR = self.ctypes.ipv4_cidr_type()
        self.IPV6 = Datatypes._IPV6 = self.ctypes.ipv6_type()
        self.ALIAS = Datatypes._ALIAS = self.ctypes.alias_type()

    def datasource_type(self, name):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        return self.ctypes.datasource_type(name)

    def external_type(self, ds_t, name):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        return self.ctypes.external_type(ds_t, name)

    def attribute_type(self, ds_t, name):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        return self.ctypes.attribute_type(ds_t, name)

    def principal_table_name(self, val):
        return self.ctypes.principal_table_name(val)

    def principal_string(self, val):
        return self.ctypes.principal_string(val)

    def address_string(self, val):
        return self.ctypes.address_string(val)

    def datasource_string(self, val):
        return self.ctypes.datasource_string(val)

    def external_string(self, val):
        return self.ctypes.external_string(val)

    def attribute_string(self, val):
        return self.ctypes.attribute_string(val)

    def set_switch_type(self, val):
        self.ctypes.set_switch_type(val)
        self.SWITCH = Datatypes._SWITCH = val

    def set_location_type(self, val):
        self.ctypes.set_location_type(val)
        self.LOCATION = Datatypes._LOCATION = val

    def set_host_type(self, val):
        self.ctypes.set_host_type(val)
        self.HOST = Datatypes._HOST = val

    def set_host_netid_type(self, val):
        self.ctypes.set_host_netid_type(val)
        self.HOST_NETID = Datatypes._HOST_NETID = val

    def set_user_type(self, val):
        self.ctypes.set_user_type(val)
        self.USER = Datatypes._USER = val

    def set_address_type(self, val):
        self.ctypes.set_address_type(val)
        self.ADDRESS = Datatypes._ADDRESS = val

    def set_group_type(self, val):
        self.ctypes.set_group_type(val)
        self.GROUP = Datatypes._GROUP = val

    def set_group_member_type(self, val):
        self.ctypes.set_group_member_type(val)
        self.GROUP_MEMBER = Datatypes._GROUP_MEMBER = val

    def set_mac_type(self, val):
        self.ctypes.set_mac_type(val)
        self.MAC = Datatypes._MAC = val

    def set_ipv4_cidr_type(self, val):
        self.ctypes.set_ipv4_cidr_type(val)
        self.IPV4_CIDR = Datatypes._IPV4_CIDR = val

    def set_ipv6_type(self, val):
        self.ctypes.set_ipv6_type(val)
        self.IPV6 = Datatypes._IPV6 = val

    def set_alias_type(self, val):
        self.ctypes.set_alias_type(val)
        self.ALIAS = Datatypes._ALIAS = val

    def set_datasource_type(self, name, val):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        self.ctypes.set_datasource_type(name, val)

    def set_external_type(self, ds_t, name, val):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        self.ctypes.set_external_type(ds_t, name, val)

    def set_attribute_type(self, ds_t, name, val):
        if isinstance(name, unicode):
            name = name.encode('utf-8')
        self.ctypes.set_attribute_type(ds_t, name, val)

    def unset_principal_type(self, val):
        self.ctypes.unset_principal_type(val)

    def unset_address_type(self, val):
        self.ctypes.unset_address_type(val)

    def unset_datasource_type(self, val):
        self.ctypes.unset_datasource_type(val)

    def unset_external_type(self, val):
        self.ctypes.unset_external_type(val)

    def unset_attribute_type(self, val):
        self.ctypes.unset_attribute_type(val)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return Datatypes(ctxt)

    return Factory()

class DatasourceInfo:
    """NOX Datasource Representation

    Attributes:
        uid          : int      - datasource-relative unique ID
        name         : unicode  - datasource name
        type_id      : int      - datasource type (defined in DatasourceType)
        search_order : int      - search priority relative to other datasources
    """

    def __init__(self):
        self.uid = None
        self.name = None
        self.type_id = None
        self.search_order = None
        
class SwitchInfo:
    """NOX Switch Representation

    Attributes:
        uid            : int                - switch-relative unique ID
        datasource_uid : int                - uid of datasource owning switch
                                              definition
        name           : unicode            - switch name
        description    : unicode            - switch description
        dpid           : datapathid         - switch datapathid binding
        external_type  : int                - external type (see ExternalType)
        locations      : list(LocationInfo) - switch's locations
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.description = None
        self.dpid = None
        self.external_type = None
        self.locations = None
        self.attributes = None

    def __str__(self):
        return "Switch: Name=%s, UID=%d, DS=%d, DPID=%s, %s" \
            % (self.name, self.uid, self.datasource_uid,
               str(self.dpid), str([str(l) for l in self.locations]))

    @staticmethod
    def principal_type():
        return Datatypes._SWITCH

class LocationInfo:
    """NOX Location Representation

    Attributes:
        uid            : int         - location-relative unique ID
        datasource_uid : int         - uid of datasource owning location def
        name           : unicode     - location name
        switch         : SwitchInfo  - location's switch
        description    : unicode     - location description
        port           : int         - physical port number on switch
        port_name      : unicode     - switch-provided name describing port
        speed          : int         - configured port speed
        duplex         : int         - configured port duplex
        auto_neg       : int         - configured port auto-negotiation
                                       operation
        neg_down       : int         - True if port is configured to negotiate
                                       speeds less than 'speed'
        admin_state    : int         - configured port state
        external_type  : int         - external type (see ExternalType)
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.switch = None
        self.description = None
        self.port = None
        self.port_name = None
        self.speed = None
        self.duplex = None
        self.auto_neg = None
        self.neg_down = None
        self.admin_state = None
        self.external_type = None
        self.attributes = None

    def __str__(self):
        return "Location: Name=%s, UID=%d, DS=%d, port=%s(%d)" \
            % (self.name, self.uid, self.datasource_uid, self.port_name,
               self.port)

    @staticmethod
    def principal_type():
        return Datatypes._LOCATION

class HostInfo:
    """NOX Host Representation

    Attributes:
        uid            : int                 - host-relative unique ID
        datasource_uid : int                 - uid of datasource owning host
                                               def
        name           : unicode             - host name
        description    : unicode             - host description
        external_type  : int                 - external type (see ExternalType)
        netids         : list(HostNetIdInfo) - host's netids
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.description = None
        self.external_type = None
        self.netids = None
        self.attributes = None

    def __str__(self):
        return "Host: Name=%s, UID=%d, DS=%d, %s" \
            % (self.name, self.uid, self.datasource_uid,
               str([str(n) for n in self.netids]))

    @staticmethod
    def principal_type():
        return Datatypes._HOST

class HostNetIdInfo:
    """NOX HostNetId Representation

    Attributes:
        uid            : int          - host netid-relative unique ID
        datasource_uid : int          - uid of datasource owning netid def
        name           : unicode      - host netid name
        host           : HostInfo     - netid's host
        description    : unicode      - host netid description
        addresses      : list(AddressInfo)  - static address binding
        is_router      : bool         - True if interface configured as router
        is_gateway     : bool         - True if interface configured as gateway
        external_type  : int          - external type (see ExternalType)
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.host = None
        self.description = None
        self.addresses = None
        self.is_router = None
        self.is_gateway = None
        self.external_type = None
        self.attributes = None

    def __str__(self):
        return "Netid: Name=%s, UID=%d, %s" \
            % (self.name, self.uid, ",".join([str(address) for 
                                              address in self.addresses]))

    @staticmethod
    def principal_type():
        return Datatypes._HOST_NETID

class UserInfo:
    """NOX User Representation

    Attributes:
        uid            : int      - user-relative unique ID
        datasource_uid : int      - uid of datasource owning user definition
        name           : unicode  - user name
        real_name      : unicode  - full user name
        description    : unicode  - user description
        location       : unicode  - user location
        phone          : unicode  - user phone number
        user_email     : unicode  - user email address
        external_type  : int      - external type (see ExternalType)
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.real_name = None
        self.description = None
        self.location = None
        self.phone = None
        self.user_email = None
        self.external_type = None
        self.attributes = None

    def __str__(self):
        return "User: Name=%s, UID=%d, DS=%d" \
            % (self.name, self.uid, self.datasource_uid)

    @staticmethod
    def principal_type():
        return Datatypes._USER

### NOT HAPPY WITH CYCLIC!

class GroupInfo:
    """NOX Group Representation

    Attributes:
        uid            : int                   - group-relative unique ID
        datasource_uid : int                   - uid of datasource owning group
                                                 def
        name           : unicode               - group name
        prince_type    : int                   - type of principal associated
                                                 with group definition
        principal_uid  : int                   - prince_type-relative uid of
                                                 associated principal
        description    : unicode               - group description
        external_type  : int                   - external type (see ExternalType)
        members        : list(GroupMemberInfo) - subset of group's members
        all_members    : bool                  - True if GroupInfo 'members'
                                                 field constructed to include 
                                                 all members. False if
                                                 constructed to include members
                                                 meeting certain criteria
                                                 (which could potentially end
                                                 up being all members).
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.name = None
        self.prince_type = None
        self.principal_uid = None
        self.description = None
        self.external_type = None
        self.members = None
        self.all_members = None
        self.attributes = None

    def __str__(self):
        return "Group: Name=%s, UID=%d, DS=%d, %s" \
            % (self.name, self.uid, self.datasource_uid,
               str([str(m) for m in self.members]))

    @staticmethod
    def principal_type():
        return Datatypes._GROUP

# should create corresponding object? (i.e. ethernetaddr, ipaddr etc)

class AddressInfo:
    """NOX Address Representation

    Attributes:
        uid            : int      - address-relative unique ID
        datasource_uid : int      - uid of datasource owning address def
        address        : unicode  - string representation of address
        address_type   : int      - address type (see AddressType)
        external_type  : int      - external type (see ExternalType)
        address_obj    : *addr    - netinet representation of address
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.address = None
        self.address_type = None
        self.external_type = None
        self.address_obj = None
        self.attributes = None

    def __str__(self):
        return "Address: Name=%s, UID=%d, DS=%d" \
            % (self.address, self.uid, self.datasource_uid)

    @staticmethod
    def principal_type():
        return Datatypes._ADDRESS

class GroupMemberInfo:
    """NOX Group Membership Representation

    Attributes:
        uid            : int    - Membership-relative unique ID
        datasource_uid : int    - uid of datasource owning membership def
        external_type  : int    - external type (see ExternalType)
        member         : *Info  - Member principal
        attributes     : list(AttributeInfo)
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.external_type = None
        self.member = None
        self.attributes = None

    def __str__(self):
        return "Member: UID=%d, DS=%d, %s" \
            % (self.uid, self.datasource_uid, str(self.member))

    @staticmethod
    def principal_type():
        return Datatypes._GROUP_MEMBER

class AttributeInfo:
    """NOX Attribute Representation

    Attributes:
        uid             : int    - Attribute-relative unique ID
        datasource_uid  : int    - uid of datasource owning attribute def
        attribute_type  : int    - attribute type
        attribute_value : int    - attribute value
        external_type   : int    - external type
    """

    def __init__(self):
        self.uid = None
        self.datasource_uid = None
        self.attribute_type = None
        self.attribute_value = None
        self.external_type = None

    def __str__(self):
        return "Attribute: UID=%d, DS=%d, t=%d, %s" \
            % (self.uid, self.datasource_uid,
               self.attribute_type, self.attribute_value)
