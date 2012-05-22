import logging 
import sets
from twisted.internet import defer
from nox.lib.directory import *
from nox.netapps.storage.storage import Storage,StorageException
from nox.netapps.bindings_storage.pybindings_storage import Name,pybindings_storage
from nox.lib.core import Component

# FIXME: These methods need to be converted to use deferreds so we
# can have better error handling should they ever throw an exception
# (in theory, they shouldn't ever throw an exception) 

BINDINGS_DIRECTORY_NAME = "Bindings Directory"
lg = logging.getLogger("bindings_directory") 


class get_matching_rows_op: 

  def get_eb(self,error): 
      lg.error("EB: get_matching_rows error: " + str(error)) 
      self.cb(self.rows) 

  def get_cb(self,res): 
    result, ctxt, row = res
    if result[0] != Storage.SUCCESS: 
      if result[0] != Storage.NO_MORE_ROWS: 
        lg.error("storage get_matching_rows error: " + str(result[1]))
      self.cb(self.rows) 
      return

    # note that an empty filter will add all rows
    for f in self.filters:
      filter_match = True
      for key,value in f.iteritems():  
        if key in row: 
          if row[key] != value:
            filter_match = False
            break
      if filter_match == True: 
        self.rows.append(row)
        break 

    d = self.ndb.get_next(ctxt) 
    d.addCallback(self.get_cb) 
    d.addErrback(self.get_eb) 

  def __init__(self, table_name, ndb, filters, cb): 
    if True:
      raise Exception('name manager removed, Doesn\'t convert database names to IDs')
    self.filters = filters
    self.cb = cb 
    self.rows = [] 
    self.ndb = ndb
    
    d = ndb.get(table_name, {}) 
    d.addCallback(self.get_cb)
    d.addErrback(self.get_eb)

class name_for_name_op: 

  def __init__(self,name_in, type_in,type_out,bs,cb):
    if True:
      raise Exception('name needs to be converted to id before can query bindings_directory')
    self.callback = cb
    self.bs = bs
    self.items = [] 
    self.type_out = type_out
    self.bs.get_entities_by_name(name_in,type_in,self.ent_callback)
     
  def ent_callback(self,ent_list):

    self.num_ents = len(ent_list)
    if self.num_ents == 0: 
      self.done() 

    self.num_callbacks = 0
    for e in ent_list:
        dpid = datapathid.from_host(e[0])
        self.bs.get_names(dpid,e[1],ethernetaddr(e[2]),e[3],self.name_callback) 

  def name_callback(self,name_list): 
    for n in name_list: 
        if n[1] == self.type_out:
          self.items.append(n[0])
    
    self.num_callbacks += 1
    if self.num_callbacks == self.num_ents: 
      self.done()

  def done(self):
    # list may contain duplicates, remove them
    l = list(sets.Set(self.items))
    self.callback(l)
    
class get_interfaces_op: 

    def __init__(self, d, hostname, bstore):
      self.d = d
      self.interfaces = [] 
      self.bstore = bstore 
      self.bstore.get_entities_by_name(hostname, Name.HOST, self.cb1)
    
    def cb1(self, netinfo_list): 
      if len(netinfo_list) == 0: 
        self.d.callback(self.interfaces) 
        return 
      
      # logic to skip a netinfo with a 0.0.0.0 IP
      # address if another with the same mac has a non-zero IP
      has_nonzero_ip = {}
      for n in netinfo_list: 
        ip = n[3]
        mac = n[2]
        if ip != 0: 
          has_nonzero_ip[mac] = True
      filtered_list = [] 
      for n in netinfo_list:
          ip = n[3]
          mac = n[2] 
          if not (ip == 0 and mac in has_nonzero_ip): 
            filtered_list.append(n)

      def create_fn(ips, macs,count):
          return lambda x: self.cb2(x, count, macs, ips)
      count = Count(len(filtered_list)) 
      for n in filtered_list:
        dpid = datapathid.from_host(n[0])
        port = n[1]
        mac_str = str(ethernetaddr(n[2]))
        ip_str = str(ipaddr(n[3]))
        f = create_fn(ip_str,mac_str,count)
        self.bstore.get_names_for_location(dpid,port,Name.LOC_TUPLE, f)

    def cb2(self, loc_tuple_list, count, mac_str, ip_str):
          if len(loc_tuple_list) > 0 :
            arr = loc_tuple_list[0][0].split("#")
            dict = {}
            dict['location_name'] = arr[0]
            dict['switch_name'] = arr[1]
            dict['port_name'] = arr[2]
            dict['dladdr'] = mac_str 
            dict['nwaddr'] = ip_str 
            self.interfaces.append(dict)
          count.n = count.n - 1
          if count.n == 0: 
            self.d.callback(self.interfaces) 
    

class BindingsDirectory(Component, Directory): 
  """Exposes active network bindings via a directory-like interface"""

  def __init__(self, ctxt):
        Component.__init__(self,ctxt) 
        self.store = None
        self.bs = None


  def install(self):
        self.store = self.resolve(Storage)
        if self.store is None:
          raise Exception("Unable to resolve required component '%s'"
                            %str(Storage))
        self.bs = self.resolve(pybindings_storage)
        if self.bs is None:
          raise Exception("Unable to resolve required component '%s'"
                            %str(pybindings_storage))

  def getInterface(self):
      return str(BindingsDirectory) 
  
  def get_bstore(self):
    return self.bs

  # the binding storage NDB tables store dpid's and
  # dladdr's as int values, so we must convert
  def fix_query_params(self,query):
        if 'dpid' in query:
          query['dpid'] = query['dpid'].as_host()
        if 'dladdr' in query:
          query['dladdr'] = query['dladdr'].hb_long()

    #  This functions retrieve principals/names based a query
    #  that specifies some set of network identifiers.  
  def search_names_by_netinfo(self, query, name_type): 
    if name_type == Name.SWITCH or name_type == Name.LOCATION:
      return self.search_location_table(query,name_type) 
    elif name_type == Name.USER or name_type == Name.HOST: 
      return self.search_id_table(query,name_type)
    raise DirectoryException("cannot query bindings of type %d" % name_type)

  def search_location_table(self,query,name_type): 
        query["name_type"] = name_type
        self.fix_query_params(query)
        d = defer.Deferred() 
        def cb(row_list):
          ret = [] 
          for r in row_list:
            ret.append(r['name'])
          d.callback(ret); 

        mr = get_matching_rows_op("bindings_location", self.store,
                                  [query],cb ) 
        return d


  def search_id_table(self, query, name_type): 
        d = defer.Deferred()
        query_set = False
        for k, v in query:
          if k == "dpid":
            query_set = True
            if "port" not in query:
              print 'don\'t support query on dp and no port'
              return
            break
          elif k == "port":
            query_set = True
            if "dpid" not in query:
              print 'don\'t support query on port and no dp'
              return
            break
          elif k == "dladdr":
            query_set = True
            break
          elif k == "nwaddr":
            query_set = True
            break

        def process_names(names):
          ret = []
          for name in names:
            if name_type == name[1]:
              ret.append(name[0])
          d.callback(ret)

        if not query_set:
          self.bs.get_all_names(name_type, process_names)
        else:
          query = query.copy()
          self.fix_query_params(query)
          self.bs.get_names_query(query, False, process_names)

        return d

  # used by webservice to get each "interface" associated
  # with a hostname.  An "interface" is
  # a (location-name, switch-name, port-name, mac, ip) 
  # tuple
  def get_interfaces_for_host(self, hostname): 
    d = defer.Deferred() 
    op = get_interfaces_op(d,hostname,self.bs) 
    return d

  # I don't think this is used anymore
  # note: same method would also work for USERS and LOCATIONS
  def get_netinfo_for_host(self, hostname): 
     
      name_type = Name.HOST
      d = defer.Deferred() 
      def cb(entity_list):
        self.convert_entity_list(entity_list,d,hostname, name_type) 

      self.bs.get_entities_by_name(hostname, name_type, cb)
      return d

  # we no longer use this for anything except hosts
  # we can probably trim it down
  def convert_entity_list(self,entity_list,d,name, name_type):

      if len(entity_list) == 0:
          d.callback(None)
          return 

      if name_type == Name.HOST:  
        h = HostInfo() 
        h.name = name
        for e in entity_list: 
          n = NetInfo() 
          n.dpid = datapathid.from_host(e[0])
          n.port = e[1]
          n.dladdr = ethernetaddr(e[2])
          n.nwaddr = e[3] 
          h.netinfos.append(n) 
        d.callback(h) 
        return
      elif name_type == Name.SWITCH: 
        if len(entity_list) == 1:
          s = SwitchInfo() 
          s.name = name
          s.dpid = datapathid.from_host(e[0])
          d.callback(s) 
        else: 
          lg.error("multiple switches found for name:" + name)
          d.callback(None)
        return 
      elif name_type == Name.LOCATION:
        if len(entity_list) == 1:
          l = LocationInfo() 
          l.name = name
          l.dpid = datapathid.from_host(e[0])
          l.port = e[1] 
          d.callback(l) 
        else: 
          lg.error("multiple locations found for name:" + name)
          d.callback(None)
        return
      else: 
        lg.error("invalid type for convert_entity_list")
         
      d.callback(None)   

  # does name-to-name lookup, for example: 
  # what hosts is user 'bob' logged into?
  # or
  # what users are at location 'marbles0'?
  # to do this, it does an entity lookup based on
  # the provided name, and then files all names
  # of the requested type associated with that entity. 
  def get_name_for_name(self, name_in, type_in, type_out, cb): 
    o = name_for_name_op(name_in,type_in, type_out, self.bs, cb) 


def getFactory():
  class Factory:
    def instance(self,ctx):
      return BindingsDirectory(ctx)
  return Factory() 


