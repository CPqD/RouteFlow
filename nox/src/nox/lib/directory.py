# Copyright 2008 (C) Nicira, Inc.
#
# This file is part of NOX.
#
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.
#

from nox.lib.netinet.netinet import *
from socket import htonl 
from nox.lib.directory_factory import Directory_Factory 
from twisted.internet import defer

"""\
Base classes for Directory access
"""

class DirectoryException(Exception):
    UNKNOWN_ERROR           = 0
    RECORD_ALREADY_EXISTS   = 1
    NONEXISTING_NAME        = 2
    OPERATION_NOT_PERMITTED = 3
    BADLY_FORMATTED_NAME    = 4
    INSUFFICIENT_INPUT      = 5
    INVALID_QUERY           = 6
    INVALID_CRED_TYPE       = 7
    INVALID_DIR_TYPE        = 8
    COMMUNICATION_ERROR     = 9
    REMOTE_ERROR            = 10
    INVALID_CONFIGURATION   = 11

    def __init__(self, message, code=UNKNOWN_ERROR):
        self.message = message
        self.code = code

    def __str__(self):
        return "%s (%s:%s) : %s" %(repr(self), self.code, self.error_str(), 
                self.message)

    def error_str(self):
        if self.code == DirectoryException.RECORD_ALREADY_EXISTS:
            return "Record already exists"
        elif self.code == DirectoryException.NONEXISTING_NAME:
            return "Name does not exist"
        elif self.code == DirectoryException.OPERATION_NOT_PERMITTED:
            return "Operation not permitted"
        elif self.code == DirectoryException.BADLY_FORMATTED_NAME:
            return "Badly formatted name"
        elif self.code == DirectoryException.INSUFFICIENT_INPUT:
            return "Insufficient input"
        elif self.code == DirectoryException.INVALID_QUERY:
            return "Invalid query"
        elif self.code == DirectoryException.INVALID_CRED_TYPE:
            return "Invalid credential type"
        elif self.code == DirectoryException.INVALID_DIR_TYPE:
            return "Invalid directory type"
        elif self.code == DirectoryException.COMMUNICATION_ERROR:
            return "Communication error"
        elif self.code == DirectoryException.REMOTE_ERROR:
            return "Remote error"
        elif self.code == DirectoryException.INVALID_CONFIGURATION:
            return "Invalid configuration"
        return "Unknown Error"

# stupid count class b/c python seems to pass ints by value
# this is useful in many scenario when you do multiple simultaneous
# callbacks and need to know when they have all finished
class Count:
    def __init__(self,n):
      self.n = n

class AuthResult:
    SUCCESS               = 0
    INVALID_CREDENTIALS   = 1
    ACCOUNT_DISABLED      = 2
    COMMUNICATION_TIMEOUT = 3
    SERVER_ERROR          = 4

    @staticmethod
    def from_dict(result_dict):
        return AuthResult(result_dict['status'], result_dict['username'],
                          result_dict['groups'], result_dict['nox_roles'])

    def __init__(self, status, username, groups=None, nox_roles=None):
        self.status = status
        self.username = username
        self.groups = set(groups or [])
        self.nox_roles = set(nox_roles or [])

    def status_str(self):
        if self.status == AuthResult.SUCCESS:
            return "Success"
        elif self.status == AuthResult.INVALID_CREDENTIALS:
            return "Invalid Credentials"
        elif self.status == AuthResult.ACCOUNT_DISABLED:
            return "Account Disabled"
        elif self.status == AuthResult.COMMUNICATION_TIMEOUT:
            return "Communication Timeout"
        elif self.status == AuthResult.SERVER_ERROR:
            return "Server Error"
        return "Unknown Error"


# These query objects are similar to the *Info objects below, 
# but are kept separate because of the simplified way in which we
# handle queries over nested attributes.  For example, a HostInfo
# object has an attribute "netinfos", which contain a list of objects
# that each could contain a "nwaddr" field.  For example:
#
# { 'name' : 'myhost', 
#   'netinfos' : [ { 'nwaddr' : "1.1.1.1" } { 'nwaddr' : "2.2.2.2" } ] 
# } 
# However, a query for a 
# host can handle take a single "nwaddr" field, which is given at 
# the top-level of the dictionary. For example:
#
# { 'nwaddr' : "2.2.2.2" } 
# 

class SwitchQuery:
    VALID_KEYS = [ "dpid", "name", "name_glob" ] 
    def __init__(self, str_dict):
      self.query_dict = {} 
      for key in SwitchQuery.VALID_KEYS: 
        if key in str_dict:
          if key == "dpid": 
            dpid_obj = datapathid.from_host(long(str_dict["dpid"])) 
            self.query_dict["dpid"] = dpid_obj
          else: 
            if isinstance(str_dict[key], basestring):
                self.query_dict[key] = str_dict[key]
            else:
                self.query_dict[key] = str(str_dict[key])
      
      if len(self.query_dict) != len(str_dict): 
        msg = "Invalid search key in switch query '%s'.  Valid keys are: %s" \
              % (str_dict, SwitchQuery.VALID_KEYS)
        raise Exception(msg)

    def get_query_dict(self): 
      return self.query_dict

class LocationQuery:
    VALID_KEYS = [ "dpid", "port", "name", "name_glob" ] 
    def __init__(self, str_dict):
      self.query_dict = {} 
      for key in LocationQuery.VALID_KEYS: 
        if key in str_dict:
          if key == "dpid": 
            dpid_obj = datapathid.from_host(long(str_dict["dpid"])) 
            self.query_dict["dpid"] = dpid_obj
          elif key == "port":
            self.query_dict["port"] = int(str_dict["port"]) 
          else: 
            if isinstance(str_dict[key], basestring):
                self.query_dict[key] = str_dict[key]
            else:
                self.query_dict[key] = str(str_dict[key])
      
      if len(self.query_dict) != len(str_dict): 
        msg = "Invalid search key in location query '%s'.  Valid keys are: %s" \
              % (str_dict, LocationQuery.VALID_KEYS)
        raise Exception(msg)

    def get_query_dict(self): 
      return self.query_dict


class HostQuery:
    VALID_KEYS = [ "dpid", "port", "dladdr", "nwaddr", "netname", "alias",
                   "name", "is_gateway", "is_router", "name_glob" ]
    def __init__(self, str_dict):
      self.query_dict = {}
      for key in HostQuery.VALID_KEYS: 
        if key in str_dict:
         if key == "dpid": 
            dpid_obj = datapathid.from_host(long(str_dict["dpid"])) 
            if (dpid_obj is None) : 
              raise Exception("'%s' is not a valid dpid" % str_dict["dpid"])
            self.query_dict["dpid"] = dpid_obj
         elif key == "port": 
            port = int(str_dict["port"]) 
            if (port is None) : 
              raise Exception("'%s' is not a valid port" % str_dict["port"])
            self.query_dict["port"] = port
         elif key == "dladdr":
            dladdr = create_eaddr(str(str_dict["dladdr"]))
            if (dladdr is None) : 
              raise Exception("'%s' is not a valid dladdr" % str_dict["dladdr"])
            self.query_dict["dladdr"] = dladdr
         elif key == "nwaddr":
            nwaddr_obj = create_ipaddr(str(str_dict["nwaddr"]))
            if (nwaddr_obj is None) : 
              raise Exception("'%s' is not a valid nwaddr" % str_dict["nwaddr"])
            self.query_dict["nwaddr"] = c_ntohl(nwaddr_obj.addr)
         else:
            if isinstance(str_dict[key], basestring):
                self.query_dict[key] = str_dict[key]
            else:
                self.query_dict[key] = str(str_dict[key])
      
      
      if len(self.query_dict) != len(str_dict): 
        msg = "Invalid search key in query '%s'.  Valid keys are: %s" \
              % (str_dict, self.VALID_KEYS)
        raise Exception(msg)

    def get_query_dict(self): 
      return self.query_dict

class HostNetNameQuery(HostQuery):
    # HostNetName queries are essentially the same as a host query,
    # where the 'name' attribute is equivalent to
    # HostNetNameInfo.mangle_host_netname(hostname, netname)
    VALID_KEYS = HostQuery.VALID_KEYS + ["hostname", "netname"]

class UserQuery:
    VALID_KEYS = [ "name", "user_id", "user_real_name", "name_glob", 
                  "description", "location", "phone", "user_email"]
    
    def __init__(self, str_dict):
      self.query_dict = {} 
      for key in UserQuery.VALID_KEYS: 
        if key in str_dict:
          if key == "user_id" or key == "password_update_epoch" \
              or key == "password_expire_epoch":
            self.query_dict[key] = int(str_dict(key))
          else: 
            if isinstance(str_dict[key], basestring):
                self.query_dict[key] = str_dict[key]
            else:
                self.query_dict[key] = str(str_dict[key])
      
      if len(self.query_dict) != len(str_dict): 
        msg = "Invalid search key in switch query '%s'.  Valid keys are: %s" \
            % (str_dict, UserQuery.VALID_KEYS)
        raise Exception(msg)

    def get_query_dict(self): 
      return self.query_dict

class GroupQuery:
    VALID_KEYS = [ "name", "name_glob" ]
    def __init__(self, str_dict):
      self.query_dict = {}
      for key in SwitchQuery.VALID_KEYS:
        if key in str_dict:
            self.query_dict[key] = str(str_dict[key])

      if len(self.query_dict) != len(str_dict):
        msg = "Invalid search key in group query '%s'.  Valid keys are: %s" \
              % (str_dict, GroupQuery.VALID_KEYS)
        raise Exception(msg)

    def get_query_dict(self):
      return self.query_dict

# abstract base class for the *Info objects below
# implements common parsing functionality 
class PrincipalInfo(object):

    # converts object to a dictionary where the values are python types
    def to_dict(self):
      dict = {} 
      for key in self.__dict__:
        if key.startswith("_"): 
          continue # ignore 'private' members
        if self.__dict__[key] is not None:
          dict[key] = self.__dict__[key] 
        
      return dict

    @staticmethod
    def from_str_dict(attr_dict):
        raise NotImplementedError("from_str_dict must be implemented in "\
                                  "subclass")

    # converts object to a dictionary where the values are all strings
    def to_str_dict(self):   
      ret = {}
      for k, v in self.__dict__.items():
        if k.startswith("_"): 
          continue # ignore 'private' members
        if v is not None:
          if isinstance(v, unicode):
            ret[k] = v.encode('utf-8')
          elif hasattr(v, '__iter__'):
              def unicode_(s, encoding):
                  if isinstance(s, unicode): return s.encode('utf-8')
                  return unicode(str(s), encoding)
              ret[k] = [unicode_(iteritem, 'utf-8') for iteritem in v]
          else:
            ret[k] = unicode(str(v), 'utf-8')
      return ret
          
    def __eq__(self, other):
        #is there a convenience method for this somewhere?
        for key in self.__dict__: 
            if not hasattr(other, key):
                return False
            if getattr(self, key) != getattr(other, key):
                return False
        return True

    def __ne__(self, other):
        return not self == other

class Credential(PrincipalInfo):
    def __init__(self, type): 
      self.type = type

    """Base class for credential Objects"""
    @staticmethod
    def from_str_dict(self):
        raise NotImplementedError("from_str_dict  must be "\
                                  "implemented in subclass")

class PasswordCredential(Credential):
    """Credential using a secret password

    PasswordCredential attributes:
        password              : str - password to put (not returned in get)
        password_expire_epoch : int - expiration time in seconds since epoch
                                      or 0 if does not expire
        password_update_epoch : int - last update time in seconds since epoch
                                      or 0 if never set
        pw_hash_type          : int - type of hashing used for password storage
        pw_salt               : str - salt used when hashing, if any
    """
    PLAIN_TEXT = 0
    HASH_MD5   = 1

    def __init__(self, password=None, password_update_epoch=None,
            password_expire_epoch=None, pw_hash_type=None, pw_salt=None):
        Credential.__init__(self, Directory.AUTH_SIMPLE) 
        self.password              = password
        self.password_update_epoch = password_update_epoch
        self.password_expire_epoch = password_expire_epoch
        self.pw_hash_type          = pw_hash_type
        self.pw_salt               = pw_salt

    @staticmethod
    def from_str_dict(attr_dict):
        s = PasswordCredential()
        if 'password' in attr_dict:
            s.password = attr_dict['password']
        if 'password_expire_epoch' in attr_dict:
            s.password_expire_epoch = attr_dict['password_expire_epoch']
        if 'password_update_epoch' in attr_dict:
            s.password_update_epoch = attr_dict['password_update_epoch']
        return s

    def to_str_dict(self):
        ret = {'type' : self.type}
        if self.password_expire_epoch is not None:
            ret['password_expire_epoch'] = str(self.password_expire_epoch)
        if self.password_update_epoch is not None:
            ret['password_update_epoch'] = str(self.password_update_epoch)
        return ret

class CertFingerprintCredential(Credential):
    """Credential holding the fingperint of a certificate 

    CertFingerprintCredential attributes:
        fingerprint              : str - the SHA1 fingperint of the certificate
                                   in all lower-case, no colons or other 
                                   separators.  For example:
                                   e92ee81df94712cb78f2e680cdbca5a328c92d84
        is_approved             : bool - inidicates whether this fingerprint
                                  has been approved as valid (i.e., trusted) 

    """
    def __init__(self, fp=None, app=False):
        Credential.__init__(self, Directory.AUTHORIZED_CERT_FP) 
        self.fingerprint              = fp
        self.is_approved              = app
    
    @staticmethod
    def from_str_dict(attr_dict):
        c = CertFingerprintCredential()
        if 'fingerprint' in attr_dict:
            c.fingerprint = attr_dict['fingerprint']
        if 'is_approved' in attr_dict:
            c.fingerprint = True
        return c

class SwitchInfo(PrincipalInfo):
    """Information record for a Switch principal

    Switch attributes:
        name      : str        - unique (hopefully user-friendly) identifier
        dpid      : datapathid - the datapathid of the switch
        locations : list[LocationInfo] - locations associated with switch
    """
    @staticmethod
    def from_str_dict(attr_dict):
        s = SwitchInfo() 
        if "name" in attr_dict: 
          s.name = attr_dict["name"]
        if "dpid" in attr_dict: 
          s.dpid = datapathid.from_host(long(attr_dict["dpid"],16)) 
        if "locations" in attr_dict and attr_dict["locations"]:
          s.locations = []
          for loc_dict in eval(attr_dict["locations"]):
            s.locations.append(LocationInfo.from_str_dict(loc_dict))
        return s
    
    def to_str_dict(self):
      dict = {}
      if self.name : dict['name'] = self.name.encode('utf-8')
      if self.dpid is not None:
        dict['dpid'] = str(self.dpid)
      if hasattr(self, 'locations') and self.locations is not None:
        dict['locations'] = [loc.to_str_dict() for loc in self.locations]
      return dict

    def __init__(self, name=None, dpid=None, locations=None):
        self.name = name
        self.dpid = dpid
        self.locations = locations or []


class LocationInfo(PrincipalInfo):
    """Information record for a Location principal

    Location attributes:
        name        : str        - unique (hopefully user-friendly) identifier
        dpid        : datapathid - the datapathid of the switch
        port        : int        - the switch port id
        port_name   : str        - string returned by switch describing
                                   physical port for port id
        speed       : int        - configured port speed
        duplex      : int        - configured port duplex
        auto_neg    : int        - configured port autonegotiation operation
        neg_down    : int        - true iff port is configured to  
                                   autonegotiate port speeds less than speed
        admin_state : int        - configured port state
    """
    SPEED_10M             = 0
    SPEED_100M            = 1
    SPEED_1G              = 2
    SPEED_10G             = 3
    DUPLEX_HALF           = 0
    DUPLEX_FULL           = 1
    AUTO_NEG_FALSE        = 0
    AUTO_NEG_TRUE         = 1
    NEGOTIATE_DOWN_FALSE  = 0
    NEGOTIATE_DOWN_TRUE   = 1
    ADMIN_STATE_DOWN      = 0
    ADMIN_STATE_UP        = 1

    @staticmethod
    def from_str_dict(attr_dict):
        l = LocationInfo() 
        if "name" in attr_dict: 
          l.name = attr_dict["name"]
        if "dpid" in attr_dict: 
          l.dpid = datapathid.from_host(long(attr_dict["dpid"])) 
        if "port" in attr_dict:
          l.port = int(attr_dict["port"]) 
        if "port_name" in attr_dict:
          l.port_name = attr_dict["port_name"] 
        if "speed" in attr_dict:
          l.speed = int(attr_dict["speed"]) 
        if "duplex" in attr_dict:
          l.duplex = int(attr_dict["duplex"]) 
        if "auto_neg" in attr_dict:
          l.auto_neg = int(attr_dict["auto_neg"]) 
        if "neg_down" in attr_dict:
          l.neg_down = int(attr_dict["neg_down"]) 
        if "admin_state" in attr_dict:
          l.admin_state = int(attr_dict["admin_state"]) 
        return l

    def to_str_dict(self):
      dict = {} 
      if self.name is not None: 
        dict['name'] = self.name.encode('utf-8')
      if self.port is not None: 
        dict['port'] = str(self.port)
      if self.dpid is not None: 
        dict['dpid'] = str(self.dpid.as_host())
      if self.port_name is not None: 
        dict['port_name'] = self.port_name.encode('utf-8')
      if self.speed is not None: 
        dict['speed'] = str(self.speed)
      if self.duplex is not None: 
        dict['duplex'] = str(self.duplex)
      if self.auto_neg is not None: 
        dict['auto_neg'] = str(self.auto_neg)
      if self.neg_down is not None: 
        dict['neg_down'] = str(self.neg_down)
      if self.admin_state is not None: 
        dict['admin_state'] = str(self.admin_state)
      return dict

    def __init__(self, name=None, dpid=None, port=None, port_name=None,
            speed=None, duplex=None, auto_neg=AUTO_NEG_TRUE,
            neg_down=NEGOTIATE_DOWN_TRUE,
            admin_state=ADMIN_STATE_UP):
        self.name = name
        self.dpid = dpid
        self.port = port
        self.port_name = port_name
        self.speed = speed
        self.duplex = duplex
        self.auto_neg = auto_neg
        self.neg_down = neg_down
        self.admin_state = admin_state


class NetInfo(PrincipalInfo):
    """Network information component of a HostInfo record

    Network Attributes:
        name       : str          - the network name of the binding
        dpid       : datapathid   - the datapathid of the switch
        port       : int          - the switch port in host byte order
        dladdr     : ethernetaddr - MAC address
        nwaddr     : int          - IP address in host byte order
        is_router  : bool         - True if dladdr is a router interface
        is_gateway : bool         - True if dladdr is a gateway interface

    For static directories, only one of dpid;port, dladdr, and nwaddr can 
    be set within a given netinfo

    is_router and is_gateway fields are only meaningful when dladdr is set
    """
    @staticmethod
    def from_str_dict(attr_dict):
        n = NetInfo()
        if "dpid" in attr_dict:
          n.dpid = datapathid.from_host(long(attr_dict["dpid"]))
          if (n.dpid is None) : raise Exception("invalid dpid")
        if "port" in attr_dict:
          n.port = int(attr_dict["port"])
          if (n.port is None) : raise Exception("invalid port")
        if "dladdr" in attr_dict:
          n.dladdr = create_eaddr(str(attr_dict["dladdr"]))
          if (n.dladdr is None) : raise Exception("invalid dladdr")
        if "nwaddr" in attr_dict:
          nwaddr_obj = create_ipaddr(str(attr_dict["nwaddr"]))
          if (nwaddr_obj is None) : raise Exception("invalid nwaddr")
          n.nwaddr = c_ntohl(nwaddr_obj.addr)
        if "is_router" in attr_dict:
          n.is_router = attr_dict["is_router"].upper() == 'TRUE'
        else:
          n.is_router = False
        if "is_gateway" in attr_dict:
          n.is_gateway = attr_dict["is_gateway"].upper() == 'TRUE'
        else:
          n.is_gateway = False
        n.name = attr_dict.get('name', '')
        return n

    # note, we need to special case almost everything b/c simply
    # calling str() won't work
    def to_str_dict(self):
      ret = {}
      if self.name:
        ret['name'] = self.name.encode('utf-8')
      else:
        ret['name'] = ''
      if self.nwaddr is  not None:
        # ipaddr constructor expects host order
        nwaddr_obj = ipaddr(self.nwaddr)
        ret['nwaddr'] = str(nwaddr_obj) 
      if self.dladdr is not None: 
        dladdr_obj = ethernetaddr(self.dladdr)
        ret['dladdr'] = str(dladdr_obj) 
      if self.dpid is not None:
        ret['dpid'] = str(self.dpid.as_host()) 
      if self.port is not None:
        ret['port'] = str(self.port)
      if self.is_router is not None:
        ret['is_router'] = str(self.is_router)
      if self.is_gateway is not None:
        ret['is_gateway'] = str(self.is_gateway)
      return ret

    def __init__(self, dpid=None, port=None, dladdr=None,
            nwaddr=None, is_router=False, is_gateway=False, name=None):
        self.name       = name or ''
        self.dpid       = dpid
        self.port       = port
        self.dladdr     = dladdr
        self.nwaddr     = nwaddr
        self.is_router  = is_router
        self.is_gateway = is_gateway

    def __str__(self):
        return "NetInfo Object: [name=%s, dpid=%s, port=%s, dladdr=%s, " \
               "nwaddr=%s is_router=%s, is_gateway=%s]" \
               %(self.name, self.dpid, self.port, self.dladdr, self.nwaddr,
               self.is_router, self.is_gateway)

    @staticmethod
    def _none_cmp(item1, item2):
        if item1 is None:
            return -1
        if item2 is None:
            return 1
        return 0
        
    def __cmp__(self, other):
        if self.name != other.name:
            if self._none_cmp(self.name, other.name) != 0:
                return self._none_cmp(self.name, other.name)
            if self.name.__lt__(other.name):
                return -1
            return 1
        if self.dpid != other.dpid:
            if self._none_cmp(self.dpid, other.dpid) != 0:
                return self._none_cmp(self.dpid, other.dpid)
            return self.dpid.as_host().__cmp__(other.dpid.as_host())
        if self.port != other.port:
            if self._none_cmp(self.port, other.port) != 0:
                return self._none_cmp(self.port, other.port)
            return self.port.__cmp__(other.port)
        if self.dladdr != other.dladdr:
            if self._none_cmp(self.dladdr, other.dladdr) != 0:
                return self._none_cmp(self.dladdr, other.dladdr)
            return self.dladdr.hb_long().__cmp__(other.dladdr.hb_long())
        if self.nwaddr != other.nwaddr:
            if self._none_cmp(self.nwaddr, other.nwaddr) != 0:
                return self._none_cmp(self.nwaddr, other.nwaddr)
            return self.nwaddr.__cmp__(other.nwaddr)
        if self.is_router != other.is_router:
            if self._none_cmp(self.is_router, other.is_router) != 0:
                return self._none_cmp(self.is_router, other.is_router)
            return self.is_router.__cmp__(other.is_router)
        if self.is_gateway != other.is_gateway:
            if self._none_cmp(self.is_gateway, other.is_gateway) != 0:
                return self._none_cmp(self.is_gateway, other.is_gateway)
            return self.is_gateway.__cmp__(other.is_gateway)
        return 0

    def is_valid_static_dir_record(self):
        #Only 1 of [dpid+port, nwaddr, dladdr] can be set in static directories
        if (self.dpid is None) ^ (self.port is None):
            return False
        set_count = 0
        if self.dpid is not None:
            set_count += 1
        if self.dladdr is not None:
            set_count += 1
        if self.nwaddr is not None:
            set_count += 1
        if set_count != 1:
            return False
        return True

    def matches_query(self, query_dict):
        if 'name' in query_dict and query_dict['name'] != self.name:
            return False
        if 'dpid' in query_dict and query_dict['dpid'] != self.dpid:
            return False
        if 'port' in query_dict and query_dict['port'] != self.port:
            return False
        if 'dladdr' in query_dict and query_dict['dladdr'] != self.dladdr:
            return False
        if 'nwaddr' in query_dict and query_dict['nwaddr'] != self.nwaddr:
            return False
        if 'is_router' in query_dict \
                and query_dict['is_router'] != self.is_router:
            return False
        if 'is_gatewan' in query_dict \
                and query_dict['is_gatewan'] != self.is_gatewan:
            return False
        return True

class HostNetNameInfo(NetInfo):
    """Network name information component of a HostInfo record
    """
    @staticmethod
    def mangle_host_netname(hostname, netname):
        return "%s^%s" %(hostname, netname)

    @staticmethod
    def demangle_host_netname(hostnetname):
        if not hasattr(hostnetname, "split") or not '^' in hostnetname:
            raise DirectoryException("Cannot demangle host netname '%s'"
                    % hostnetname, DirectoryException.BADLY_FORMATTED_NAME)
        return tuple(hostnetname.split('^', 1))

    def __init__(self, hostname=None, netname=None):
        self.hostname = hostname or ''
        self.netname = netname or ''

    def __getattribute__(self, name):
        if name == 'name':
            return self.mangle_host_netname(self.hostname, self.netname)
        return object.__getattribute__(self, name)
        raise AttributeError(item)

    @staticmethod
    def from_str_dict(attr_dict):
        if not 'hostname' in attr_dict:
            raise Exception("missing required hostname")
        return HostNetNameInfo(attr_dict['hostname'],
                               attr_dict.get('netname', ''))

    def to_str_dict(self):
        ret = super().to_str_dict()
        ret['netname'] = self.netname.encode('utf-8')
        ret['hostname'] = self.hostname.encode('utf-8')
        ret['name'] = self.name.encode('utf-8')
        return ret

    def __cmp__(self, other):
        if self.name != other.name:
            if self._none_cmp(self.name, other.name) != 0:
                return self._none_cmp(self.name, other.name)
            if self.name.__lt__(other.name):
                return -1
            return 1
        if self.hostname != other.hostname:
            if self._none_cmp(self.hostname, other.hostname) != 0:
                return self._none_cmp(self.hostname, other.hostname)
            if self.hostname.__lt__(other.hostname):
                return -1
            return 1
        return 0


class HostInfo(PrincipalInfo):
    """Information record for a Host principal

    Host Attributes:
        name        : str           - unique user-friendly identifier
        description : str           - arbitrary descriptive text field
        aliases     : list(str)     - additional names for host
        netinfos    : list(NetInfo) - network info associated with host
    """

    @staticmethod
    def from_str_dict(attr_dict):
      h = HostInfo()
      if "name" in attr_dict: 
        h.name = attr_dict["name"]
      if "description" in attr_dict: 
        h.description = attr_dict["description"]
      netinfo_dicts = attr_dict.get("netinfos", []) 
      for n_dict in netinfo_dicts: 
          newinfo = NetInfo.from_str_dict(n_dict)
          h.netinfos.append(newinfo)
      return h
    
    def __init__(self, name=None, description=None, aliases=None,
            netinfos=None):
        self.name        = name
        self.description = description
        self.aliases     = aliases or []
        self.netinfos    = netinfos or []
    
    def __eq__(self, other):
        for key in self.__dict__: 
            if not hasattr(other, key):
                return False
            if key == 'netinfos':
                if sorted(self.netinfos) != sorted(other.netinfos):
                    return False
            elif getattr(self, key) != getattr(other, key):
                return False
        return True

    def to_dict(self):
      dict = {} 
      dict['name'] = self.name
      if self.description and self.description != '': 
        dict['description'] = self.description
      l = [] 
      for ni in self.netinfos:
        l.append(ni.to_dict()) 
      dict['netinfos'] = l
      dict['aliases'] = self.aliases[:]
      return dict
    
    def to_str_dict(self):
      dict = {}
      dict['name'] = self.name.encode('utf-8')
      if self.description and self.description != '':
        dict['description'] = self.description.encode('utf-8')
      dict['netinfos'] = [ni.to_str_dict() for ni in self.netinfos]
      dict['aliases'] = [alias.encode('utf-8') for alias in self.aliases]
      return dict


class UserInfo(PrincipalInfo):
    """Information record for a User principal

    User Attributes:
        name                  : str - unique user name of user
        user_id               : int - unique id number of user
        user_real_name        : str - real name of user
        description           : str - descriptive text
        location              : str - location information for user
        phone                 : str - contact information for user
        user_email            : str - user email address
    """
    @staticmethod
    def from_str_dict(attr_dict):
        u = UserInfo()
        for key in attr_dict.keys(): 
          if key not in u.__dict__: 
            raise Exception("invalid key: " + key)
          if key == 'user_id':
            u.__dict__[key] = int(attr_dict[key])
          else:
            u.__dict__[key] = attr_dict[key] 
        return u 

    def __init__(self, name=None, user_id=None, user_real_name=None, 
                 description=None, location=None, phone=None, user_email=None):
        self.name                  = name
        self.user_id               = user_id
        self.user_real_name        = user_real_name
        self.description           = description
        self.location              = location
        self.phone                 = phone
        self.user_email            = user_email


class GroupInfo(PrincipalInfo):
    """Information record for a Switch, Location, Host, or User group

    Group Attributes:
        name           : str       - unique group name
        description    : str       - Description of group
        member_names   : list(str) - all group member names
        subgroup_names : list(str) - all subgroup names
    """
    @staticmethod
    def from_str_dict(attr_dict):
      gi = GroupInfo() 
      if "name" in attr_dict: 
        gi.name = attr_dict["name"] 
      if "description" in attr_dict: 
        gi.description = attr_dict["description"] 
      return gi 

    def to_str_dict(self):
      ret = {}
      if self.name : ret['name'] = self.name.encode('utf-8')
      if self.description is not None: ret['description'] = self.description
      ret['member_names'] = []
      for member in self.member_names:
        if isinstance(member, cidr_ipaddr):
            if member.get_prefix_len() == 32:
                ret['member_names'].append(str(member)[:-3])
            else:
                ret['member_names'].append(str(member))
        elif isinstance(member, unicode):
            ret['member_names'].append(member.encode('utf-8'))
        else:
            ret['member_names'].append(str(member))
      ret['subgroup_names'] = []
      for sg in self.subgroup_names:
        if isinstance(sg, unicode):
          ret['subgroup_names'].append(sg.encode('utf-8'))
        else:
          ret['subgroup_names'].append(sg)
      return ret

    def __init__(self, name=None, description=None, member_names=None,
                 subgroup_names=None):
        self.name           = name
        self.description    = description
        #TODO: members should be a set, not a list/tuple !!!
        self.member_names   = member_names or []
        assert(type(self.member_names) == list or
               type(self.member_names) == tuple)
        self.subgroup_names = subgroup_names or []
        assert(type(self.subgroup_names) == list or
               type(self.subgroup_names) == tuple)

    def __eq__(self, other):
        if self.name != other.name:
            return False
        if self.description != other.description:
            return False
        if len(set(self.member_names) ^ set(other.member_names)):
            return False
        if len(set(self.subgroup_names) ^ set(other.subgroup_names)):
            return False
        return True

# for now, directories are by default their own factories.
# This should change soon.  
class Directory(Directory_Factory):
    """Defines the interface for classes that perform directory operations.

    All methods may return None if an unexpected error occurs
    """
    
    def __init__(self):
        self._enabled_auth_types = self.supported_auth_types()

        self._enabled_principals = {
            Directory.SWITCH_PRINCIPAL       :
                self.principal_supported(Directory.SWITCH_PRINCIPAL),
            Directory.LOCATION_PRINCIPAL     :
                self.principal_supported(Directory.LOCATION_PRINCIPAL),
            Directory.HOST_PRINCIPAL         :
                self.principal_supported(Directory.HOST_PRINCIPAL),
            Directory.USER_PRINCIPAL         :
                self.principal_supported(Directory.USER_PRINCIPAL),
            Directory.HOST_NETNAME_PRINCIPAL :
                self.principal_supported(Directory.HOST_NETNAME_PRINCIPAL),
        }

        self._enabled_groups = {
            Directory.SWITCH_PRINCIPAL_GROUP   : 
                self.group_supported(Directory.SWITCH_PRINCIPAL_GROUP) and \
                self.principal_enabled(Directory.SWITCH_PRINCIPAL),
            Directory.LOCATION_PRINCIPAL_GROUP : 
                self.group_supported(Directory.LOCATION_PRINCIPAL_GROUP) and \
                self.principal_enabled(Directory.LOCATION_PRINCIPAL),
            Directory.HOST_PRINCIPAL_GROUP     : 
                self.group_supported(Directory.HOST_PRINCIPAL_GROUP) and \
                self.principal_enabled(Directory.HOST_PRINCIPAL),
            Directory.USER_PRINCIPAL_GROUP     :
                self.group_supported(Directory.USER_PRINCIPAL_GROUP) and \
                self.principal_enabled(Directory.USER_PRINCIPAL),
            Directory.DLADDR_GROUP       :
                self.group_supported(Directory.DLADDR_GROUP),
            Directory.NWADDR_GROUP       :
                self.group_supported(Directory.NWADDR_GROUP),
            Directory.HOST_NETNAME_GROUP :
                self.group_supported(Directory.HOST_NETNAME_GROUP),
        }


    ## -- 
    ## Meta information
    ## --

    def get_type(self):
        """Return string describing directory component type

        Subclasses may wish to override to provide a more descriptive string
        """
        return self.__class__.__name__

    def get_config_params(self):
        """Return string->string dictionary of configuration parameters

        Call is synchronous (does NOT return deferred)
        """
        raise NotImplementedError("get_config_params must be implemented in "\
                                  "subclass")

    def set_config_params(self, param_dict):
        """Update parameters specified by keys in param_dict to
        associated values

        Returns deferred returning the new parameters
        """
        raise NotImplementedError("set_config_params must be implemented in "\
                                  "subclass")

    class DirectoryStatus:
        UNKNOWN = 0
        OK      = 1  # AKA 'working'
        INVALID = 2  # AKA 'not working'

        def __init__(self, status, message=''):
            self.status = status
            self.message = message

    def get_status(self):
        """Return DirectoryStatus describing current state of directory
        """
        return Directory.DirectoryStatus(Directory.DirectoryStatus.UNKNOWN,
                "Directory status not reported by directory type '%s'"
                %self.get_type())

    ## --
    ## Authentication
    ## --

    def get_enabled_auth_types(self):
        """Returns tuple of authentication types the underlying directory
        instance is configured to support
        """
        return self._enabled_auth_types

    def set_enabled_auth_types(self, auth_type_tuple):
        """Set the authentication types that should be used by the
        underlying directory

        Returns deferred returning the new enabled auth types.
        """
        #TODO persist
        self._enabled_auth_types = auth_type_tuple[:]
        return defer.succeed(auth_type_tuple[:])

    def get_credentials(self, principal_type, principal_name, cred_type=None):
        """Reterns a deferred called with a list of Credential objects 
        representing all credentials of 'cred_type' associated with 
        the pricipal principal (list may be empty) 

        If 'cred_type' is None, credentials of all types are returned.
        """
        raise NotImplementedError("get_credentials must be implemented in "\
                                  "subclass")

    def put_credentials(self, principal_type, principal_name, cred_list, \
                            cred_type=None):
        """Set credentials for specified principal.  'cred_list' is a
        list of Credential objects

        If 'cred_type' is None, replaces all existing credentials for
        the principal

        If 'cred_type' is not none, replaces all existing credentials of that
        type for the principal.  A DirectoryException will be raised if
        'cred_type' is not None and does not match the type of all
        credentials in 'cred_list'.

        Returns deferred call with the new credential list
        """
        raise NotImplementedError("put_credentials must be implemented in "\
                                  "subclass")

    def simple_auth(self, username, password):
        """Perform a simple username/password authentication.

        Arguments:
          username: login name of the user
          password: password provided by the user

        Returns:
          Deferred object returning an AuthResult class
        """
        raise NotImplementedError("simple_auth must be implemented in "\
                                  "subclass")

    def get_user_roles(self, username, local_groups=None):
        """Return deferred returing set of all roles for the specified user

        local_groups argument is only meaningful for directories supporting
        global groups (supports_global_groups() returns True)
        """
        return defer.succeed(set())

    ## --
    ## Network topology properties
    ## --

    def is_gateway(self, dladdr):
        """Returns deferred returning True if dladdr is configured to be \
        a network gateway, False if not, or None if dladdr is not
        registered in the directory
        """
        raise NotImplementedError("is_gateway must be implemented in "\
                                  "subclass")

    def is_router(self, dladdr):
        """Returns deferred returning True if dladdr is configured to be \
        a network router, False if not, or None if dladdr is not
        registered in the directory
        """
        raise NotImplementedError("is_router must be implemented in "\
                                  "subclass")

    # --
    # Principal Registration (Switches, Locations, Hosts, and Users)
    #
    # Subclasses may implement EITHER the general principal and
    # principal group methods OR the methods specific to each principal
    # type.
    #
    # If <principal>_supported returns READ_ONLY_SUPPORT, the associate "get"
    # methods must be implemented.  If <principal>_supported returns
    # READ_WRITE_SUPPORT, all associated methods must be implemented.
    #
    # For principals supporting groups, the associated group methods
    # must also be implemented.
    #
    # All methods (other than *_enabled, *_supported) return a deferred;
    # when deferred is fired:
    #   callback indicates operation successful (Yay!) 
    #   errback indicates error * aww :( *
    #
    # TODO: 
    #  - search_* methods will need to use Query objects in the future
    # --

    # --
    # General Principal Methods
    # --

    @staticmethod
    def _compare_support(supp1, supp2):
        if supp1 == supp2:
            return 0
        elif supp1 == self.READ_WRITE_SUPPORT:
            return 1
        elif supp1 == self.NO_SUPPORT:
            return -1
        elif supp2 == self.NO_SUPPORT:
            return 1
        else:
            return -1

    def set_enabled_principals(self, enabled_principal_dict):
        """Returns deferred returning the new enabled dict
        """
        #TODO: persist
        for key, val in enabled_principal_dict.items():
            self._enabled_principals[key] = val
        for pg, p in self.PRINCIPAL_GROUP_TO_PRINCIPAL.iteritems():
            self._enabled_groups[pg] = self._enabled_principals[p]
        return defer.succeed(self._enabled_principals.copy())

    def get_enabled_principals(self):
        return self._enabled_principals.copy()

    def principal_enabled(self, principal_type):
        """Returns one of NO_SUPPORT, READ_ONLY_SUPPORT, or READ_WRITE_SUPPORT
           depending on the configuration of the directory instance.
        """ 
        return self._enabled_principals[principal_type]

    def add_principal(self, principal_type, principal_info):
        """Add name<->dpid binding to directory

        Fails with DirectoryException(code=RECORD_ALREADY_EXISTS) if
        principal_info with same name already exists

        Returns deferred returning PrincipalInfo describing the principal added
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.add_switch(principal_info)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.add_location(principal_info)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.add_host(principal_info)
        elif principal_type == self.USER_PRINCIPAL:
            return self.add_user(principal_info)
        else:
            raise NotImplementedError("add_principal does not support "\
                                      "principal type '%s'" %principal_type)

    def modify_principal(self, principal_type, principal_info):
        """Update name<->dpid binding for principal

        Fails if principal_info with same name does not exist

        Returns deferred returning PrincipalInfo describing the updated
        principal
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.modify_switch(principal_info)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.modify_location(principal_info)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.modify_host(principal_info)
        elif principal_type == self.USER_PRINCIPAL:
            return self.modify_user(principal_info)
        else:
            raise NotImplementedError("modify_principal does not support "\
                                      "principal type '%s'" %principal_type)

    def rename_principal(self, principal_type, old_name, new_name):
        """Rename principal named old_name to new_name

        Returns deferred returning principal_info describing renamed principal

        Fails if principal with name same as new_name already exists
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.rename_switch(principal_info)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.rename_location(principal_info)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.rename_host(principal_info)
        elif principal_type == self.USER_PRINCIPAL:
            return self.rename_user(principal_info)
        else:
            raise NotImplementedError("rename_principal does not support "\
                                      "principal type '%s'" %principal_type)

    def del_principal(self, principal_type, principal_name):
        """Remove principal binding with name 'principal_name' from directory

        Returns deferred returning PrincipalInfo describing principal deleted
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.del_switch(principal_name)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.del_location(principal_name)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.del_host(principal_name)
        elif principal_type == self.USER_PRINCIPAL:
            return self.del_user(principal_name)
        else:
            raise NotImplementedError("del_principal does not support "\
                                      "principal type '%s'" %principal_type)

    def get_principal(self, principal_type, principal_name, *args, **vargs):
        """Get information for principal with name 'principal_name'

        Returns deferred returning PrincipalInfo instance or None if
        principal_name does not exist
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.get_switch(principal_name, *args, **vargs)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.get_location(principal_name, *args, **vargs)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.get_host(principal_name, *args, **vargs)
        elif principal_type == self.USER_PRINCIPAL:
            return self.get_user(principal_name, *args, **vargs)
        elif principal_type == self.HOST_NETNAME_PRINCIPAL:
            return self.get_host_netname(principal_name, *args, **vargs)
        else:
            raise NotImplementedError("get_principal does not support "\
                                      "principal type '%s'" %principal_type)

    def search_principals(self, principal_type, query_dict):
        """Returns deferred returning list of principal names

        Raises DirectoryException on invalid query parameter
        """
        if principal_type == self.SWITCH_PRINCIPAL:
            return self.search_switches(query_dict)
        elif principal_type == self.LOCATION_PRINCIPAL:
            return self.search_locations(query_dict)
        elif principal_type == self.HOST_PRINCIPAL:
            return self.search_hosts(query_dict)
        elif principal_type == self.USER_PRINCIPAL:
            return self.search_users(query_dict)
        elif principal_type == self.HOST_NETNAME_PRINCIPAL:
            return self.search_host_netnames(query_dict)
        else:
            raise NotImplementedError("search_principals does not support "\
                                      "principal type '%s'" %principal_type)

    # --
    # General Principal Group Methods
    # --

    def set_enabled_groups(self, enabled_groups_dict):
        for gt, supp in enabled_groups_dict.items():
            if self.PRINCIPAL_GROUP_TO_PRINCIPAL.has_key(gt):
                raise Exception("Principal group enable status is tied "
                        "to the enable status of the associated principal")
            self._enabled_groups[gt] = supp

    def get_enabled_groups(self):
        return self._enabled_groups.copy()

    def group_enabled(self, group_type):
        return self._enabled_groups[group_type]

    def get_group_membership(self, group_type, member_name, local_groups=None):
        """Return deferred returning list of principal group names that \
        member_name is a member of

        local_groups argument is only meaningful for directories supporting
        global groups (supports_global_groups() returns True)
        """
        raise NotImplementedError("get_group_membership does not support "
                                  "group type '%s'" %group_type)

    def search_groups(self, group_type, query_dict):
        """Returns deferred returning list of group names

        Raises DirectoryException on invalid query parameter
        """
        raise NotImplementedError("search_groups does not support group "
                                  "type '%s'" %group_type)

    def get_group(self, group_type, group_name):
        """Return deferred returning GroupInfo instance
        """
        raise NotImplementedError("get_group does not support group type "
                                  "'%s'" %group_type)

    def get_group_parents(self, group_type, group_name):
        """Return deferred returning list of principal group names of
        which group_name is a direct subgroup.
        """
        raise NotImplementedError("get_group_parents does not support group "
                                  "type '%s'" %group_type)

    def add_group(self, group_type, group_info):
        """Creates new principal group

        Returns deferred returning a GroupInfo object representing the
        new group

        Fails if principal group with same name already exists
        """
        raise NotImplementedError("add_group does not support group type "
                                  "'%s'" %group_type)

    def modify_group(self, group_type, group_info):
        """Update description for group

        Fails if group_info with same name does not exist

        Note: member_names and subgroup_names on group_info are ignored; use
              add/del_group_members instead

        Returns deferred returning GroupInfo describing the updated group
        """
        raise NotImplementedError("modify_group does not support group type "
                                  "'%s'" %group_type)

    def rename_group(self, group_type, old_name, new_name):
        """Rename group named old_name to new_name

        Returns deferred returning GroupInfo describing renamed group

        Fails if group of type group_type with name same as new_name
        already exists
        """
        raise NotImplementedError("rename_group does not support group type "
                                  "'%s'" %group_type)

    def del_group(self, group_type, group_name):
        """Remove principal group and all membership records

        Returns deferred returning a GroupInfo object representing the
        deleted group
        """
        raise NotImplementedError("del_group does not support group type "
                                  "'%s'" %group_type)

    def add_group_members(self, group_type, group_name,
            member_names=[], subgroup_names=[]):
        """Add principal names and principal subgroup names to principal group

        Returns deferred returning tuple containing the added member_names
        and subgroup_names
        """
        raise NotImplementedError("add_group_members does not support group "
                                  "type '%s'" %group_type)

    def del_group_members(self, group_type, group_name,
            member_names=[], subgroup_names=[]):
        """Remove principal names and principal subgroup names from
        principal group

        Returns deferred returning tuple containing the deleted member_names
        and subgroup_names
        """
        raise NotImplementedError("del_principal_group_members does not "
                                  "support group type '%s'" %group_type)

    # -- 
    # Switches
    # -- 

    def add_switch(self, switch_info):
        """Add name<->dpid binding to directory

        Also adds LocationInfo objects if 'locations' field is not empty

        Fails if switch_info with same name already exists

        Returns deferred returning SwitchInfo representing the switch added
        """
        raise NotImplementedError("add_switch must be implemented in "\
                                  "subclass")

    def modify_switch(self, switch_info):
        """Update name<->dpid binding for switch

        Fails if switch_info with same name does not exist

        Returns deferred returning SwitchInfo representing the updated switch
        """
        raise NotImplementedError("modify_switch must be implemented in "\
                                  "subclass")

    def rename_switch(self, old_name, new_name):
        """Rename switch named old_name to new_name

        Also updates all location records beginning with old_name to
        begin with new_name.  The 'locations' field in the resulting 
        SwitchInfo instance will be populated with LocationInfo objects 
        of the renamed locations.  Each LocationInfo object will have a
        '_old_name' memeber containing the previous name.

        Fails if switch with name same as new_name already exists

        Returns deferred returning SwitchInfo instance.
        """
        raise NotImplementedError("rename_switch must be implemented in "\
                                  "subclass")

    def del_switch(self, switch_name):
        """Remove switch binding with name 'switch_name' from directory.

        Also removes all associated locations.  The 'locations' field in
        the resulting SwitchINfo instance will be populated with the
        LocationInfo objects of the deleted locations.

        Returns deferred returning SwitchInfo representing the switch deleted
        """
        raise NotImplementedError("del_switch must be implemented in subclass")

    def get_switch(self, switch_name, include_locations=False):
        """Get information for switch with name 'switch_name'

        The 'locations' field in the resulting SwitchInfo instance will
        be populated only if include_locations is True
        
        Returns deferred returning SwitchInfo instance or None if
        switch_name does not exist
        """
        raise NotImplementedError("get_switch must be implemented in "\
                                  "subclass")

    def search_switches(self, query_dict):
        """Returns deferred returning list of switch names

        Raises DirectoryException on invalid query parameter
        """
        raise NotImplementedError("search_switches must be implemented in "\
                                  "subclass")

    # -- 
    # Locations
    # -- 
    
    def add_location(self, location_info):
        """Add name<->dpid,port binding to directory

        Fails if location_info with same name already exists

        Returns deferred returning LocationInfo representing the location added
        """
        raise NotImplementedError("add_location must be implemented in "\
                                  "subclass")

    def modify_location(self, location_info):
        """Update name<->dpid,port binding for location named 'name'

        Fails if location_info with same name does not exist

        Returns deferred returning LocationInfo representing the updated
        location
        """
        raise NotImplementedError("modify_location must be implemented in "\
                                  "subclass")

    def rename_location(self, old_name, new_name):
        """Rename location named old_name to new_name

        Returns deferred

        Fails if location with name same as new_name already exists
        """
        raise NotImplementedError("rename_location must be implemented in "\
                                  "subclass")

    def del_location(self, location_name):
        """Remove location named 'location_name' from directory.

        Returns deferred returning LocationInfo representing the location 
        deleted
        """
        raise NotImplementedError("del_location must be implemented in "\
                                  "subclass")

    def get_location(self, location_name):
        """Get information for location with name 'location_name'

        Returns deferred returning LocationInfo instance or None if
        location_name does not exist
        """
        raise NotImplementedError("get_location must be implemented in "\
                                  "subclass")

    def search_locations(self, query_dict):
        """Returns deferred returning list of location names

        Raises DirectoryException on invalid query parameter
        """
        raise NotImplementedError("search_locations must be implemented in "\
                                  "subclass")

    # -- 
    # Hosts
    # -- 

    def add_host(self, host_info):
        """Add host with specified attributes to directory

        Fails if host_info with same name already exists

        Returns deferred returning HostInfo representing the host added
        """
        raise NotImplementedError("add_host must be implemented in "\
                                  "subclass")

    def modify_host(self, host_info):
        """Replace host record with matching name field with provided record

        Fails if host_info with same name does not exist

        Returns deferred returning HostInfo representing the updated host
        """
        raise NotImplementedError("modify_host must be implemented in "\
                                  "subclass")

    def rename_host(self, old_name, new_name):
        """Rename host named old_name to new_name

        Returns deferred

        Fails if host with name same as new_name already exists
        """
        raise NotImplementedError("rename_host must be implemented in "\
                                  "subclass")

    def del_host(self, name):
        """Remove host named 'name' from directory.

        Returns deferred returning HostInfo representing the host deleted
        """
        raise NotImplementedError("del_host must be implemented in "\
                                  "subclass")

    def get_host(self, host_name):
        """Get information for host with name 'host_name'

        Returns deferred returning HostInfo instance or None if
        host_name does not exist
        """
        raise NotImplementedError("get_host must be implemented in "\
                                  "subclass")

    def search_hosts(self, query_dict):
        """Returns deferred returning list of host names

        Raises DirectoryException on invalid query parameter
        """
        raise NotImplementedError("search_hosts must be implemented in "\
                                  "subclass")

    # -- 
    # Users
    # -- 

    def add_user(self, user_info):
        """Add user named 'name' with specified attributes to directory

        Fails if user_info with same name already exists

        Returns deferred returning UserInfo representing the user added
        """
        raise NotImplementedError("add_user must be implemented in "\
                                  "subclass")

    def modify_user(self, user_info):
        """Replace user record with matching name field with provided record

        Fails if user_info with same name does not exist

        Returns deferred returning UserInfo representing the updated user
        """
        raise NotImplementedError("modify_user must be implemented in "\
                                  "subclass")

    def rename_user(self, old_name, new_name):
        """Rename user named old_name to new_name

        Returns deferred

        Fails if user with name same as new_name already exists
        """
        raise NotImplementedError("rename_user must be implemented in "\
                                  "subclass")

    def del_user(self, user_name):
        """Remove user named 'user_name' from directory.

        Returns deferred returning UserInfo representing user deleted
        """
        raise NotImplementedError("del_user must be implemented in "\
                                  "subclass")

    def get_user(self, user_name):
        """Get information for user with name 'user_name'

        Return deferred returning UserInfo instance or None if user_name
        does not exist
        """
        raise NotImplementedError("get_user must be implemented in "\
                                  "subclass")

    def search_users(self, query_dict):
        """Return deferred returning list of user names

        Raises DirectoryException on invalid query parameter
        """
        raise NotImplementedError("search_users must be implemented in "\
                                  "subclass")

    # -- 
    # HostNetNames
    # -- 

    def get_host_netname(self, host_netname_name):
        """Get information for host_netname with name 'host_netname_name'

        Return deferred returning HostNetNameInfo instance or None if
        host_netname_name does not exist
        """
        def _parse_result(match):
            if match:
                return HostNetNameInfo(hname, nname)
            return None
        hname, nname = HostNetNameInfo.demangle_host_netname(host_netname_name)
        d = self.search_principals(Directory.HOST_PRINCIPAL,
                {'name' : hname, 'netname' : nname})
        d.addCallback(_parse_result)
        return d

    def search_host_netnames(self, query_dict):
        """Return deferred returning list of host network names

        Raises DirectoryException on invalid query parameter
        """
        def _get_hosts(matches):
            dlist = [self.get_principal(Directory.HOST_PRINCIPAL, hname)
                    for hname in matches]
            return defer.DeferredList(dlist, fireOnOneErrback=True)

        def _check_netinfos(resultList):
            ret = []
            hosts = [kv[1] for kv in resultList]
            niquery = query_dict.copy()
            if 'name' in niquery: del niquery['name']
            if 'netname' in niquery:
                niquery['name'] = niquery['netname']
                del niquery['netname']
            for host in hosts:
                if host is None:
                  continue # a host may have disappeared between the 
                           # the search_principals and get_principal calls
                for ni in host.netinfos:
                    if ni.matches_query(niquery):
                        ret.append(HostNetNameInfo.mangle_host_netname(
                                host.name, ni.name))
            return ret
        if 'name' in query_dict:
            hname, nname = HostNetNameInfo.demangle_host_netname(
                    query_dict.pop('name'))
            if 'hostname' in query_dict and query_dict['hostname'] != hname:
                return None
            if 'netname' in query_dict and query_dict['netname'] != nname:
                return None
            query_dict['name'] = hname
            query_dict['netname'] = nname
        if 'hostname' in query_dict:
            query_dict['name'] = query_dict.pop('hostname')
        d = self.search_principals(Directory.HOST_PRINCIPAL, query_dict)
        d.addCallback(_get_hosts)
        d.addCallback(_check_netinfos)
        return d

