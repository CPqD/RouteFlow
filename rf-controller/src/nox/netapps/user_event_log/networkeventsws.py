# -*- coding: utf8 -*-
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

from nox.lib.core import *
from twisted.python.failure import Failure
from nox.webapps.webserver.webauth import Capabilities
from nox.ext.apps.commonui.authui import UISection, UIResource
from nox.webapps.webserver import webauth
from nox.webapps.webserver.webserver import *
from nox.webapps.webservice.webservice import *
from nox.netapps.user_event_log.pyuser_event_log import pyuser_event_log, \
    LogLevel, LogEntry
from nox.netapps.bindings_storage.pybindings_storage import pybindings_storage
from nox.webapps.webservice.webservice import json_parse_message_body
from nox.lib.netinet.netinet import *
from nox.webapps.webservice.web_arg_utils import *
from twisted.internet.defer import Deferred
from nox.netapps.data.datatypes_impl import Datatypes
from nox.netapps.data.datacache_impl import DataCache

import simplejson
import types
import copy
import re

# matches each instance of a format string, to be used with
# fmt_pattern.findall(log_message) to get a list of all format
# strings used in a log message
fmt_pattern = re.compile('{[a-z]*}') 

lg = logging.getLogger("networkeventsws")

# makes sure a path component is a currently valid logid value
class WSPathValidLogID(WSPathComponent): 
  def __init__(self, uel):
    WSPathComponent.__init__(self)
    self.uel = uel 

  def __str__(self):
    return "<logid>"

  def extract(self, pc, data): 
    if pc == None:
      return WSPathExtractResult(error="End of requested URI")
    try: 
      max_logid = self.uel.get_max_logid()
      logid = long(pc) 
      if logid > 0 and logid <= max_logid: 
        return WSPathExtractResult(value=pc)
    except:
      pass
    e = "Invalid LogID value '" + pc + "'. Must be number 0 < n <= " \
                                        + str(max_logid)
    return WSPathExtractResult(error=e)

def string_for_name_type(datatypes, type, is_plural): 
  s = ""
  if type == datatypes.USER: s = "user"
  elif type == datatypes.HOST: s = "host"
  elif type == datatypes.LOCATION: s = "location"
  elif type == datatypes.SWITCH: s = "switch"
  elif type == datatypes.USER_GROUP: s = "user group"
  elif type == datatypes.HOST_GROUP: s = "host group"
  elif type == datatypes.LOCATION_GROUP: s = "location group"
  elif type == datatypes.SWITCH_GROUP: s = "switch group"

  if is_plural: 
    if type == datatypes.SWITCH: 
      s += "es"
    else:
      s += "s" 
  
  return s

def get_matching_ids(type, all_ids): 
  for_use = [] 
  for n in all_ids:
    if(n["type"] == type):
        for_use.append((n["id"],n["type"]))
  return for_use


def get_name_str(ids, datacache): 
  for_use = [] 
  for n in ids:
      for_use.append(datacache.get_name(n[1], n[0]))
  if len(for_use) == 0:
    for_use.append("<unknown>") 
  
  n = ""
  for i in range(len(for_use)): 
      n += str(for_use[i])
      if i < len(for_use) - 1:
        n += ","
  return "'%s'" % n

     
def fill_in_msg(uel, datatypes, datacache, msg, src_names, dst_names):
  fmts_used = fmt_pattern.findall(msg)
  fmts_used = map(lambda s: s[1:-1],fmts_used) # remove braces

  ids = [] 
  for fmt in fmts_used: 
    if fmt not in uel.principal_format_map: 
      lg.error("invalid format string '%s' in message '%s'" % (fmt,msg))
      continue

    name_type,dir = uel.principal_format_map[fmt] 
    if dir == LogEntry.SRC:
      name_list = src_names
    else:
      name_list = dst_names
    matching_ids = get_matching_ids(name_type, name_list)
    name = get_name_str(matching_ids, datacache) 
    msg = msg.replace("{"+fmt+"}", name)
    if len(matching_ids) == 1: 
      ids.append(matching_ids[0])
    else:
      ids.append((-1,0)) 
  return (msg, ids)

def make_entry(uel, datatypes, datacache, logid, ts, app, 
                    level, msg, src_names, dst_names):
  msg,ids = fill_in_msg(uel, datatypes, datacache, msg,src_names,dst_names)
  return { "logid" : logid, 
            "timestamp" : ts, 
            "app" : app, 
            "level" : level, 
            "msg" : msg, 
            "ids" : ids
         }

def err(failure, request, fn_name, msg):
  lg.error('%s: %s' % (fn_name, str(failure)))
  return internalError(request, msg)

def dump_list_to_json(ret_list,request): 
    request.write(simplejson.dumps({ 
                  "identifier" : "logid",
                  "items" : ret_list
                  } ))
    request.finish()

# get all log entries associated with a 'name' (ie a host or user)
# uses get_logids_for_name() and then uses process_list_op
class process_name_op: 

  def __init__(self, uel,datatypes,datacache): 
    self.uel = uel
    self.datatypes = datatypes
    self.datacache = datacache

  def start(self, uid, principal_type, filter): 
    self.filter = filter
    self.d = Deferred()
    self.uel.get_logids_for_name(uid,principal_type,self.callback)
    return self.d

  def callback(self,logids):
      p = process_list_op(self.uel,self.datatypes,self.datacache)
      list_op_d = p.start(logids, self.filter)
      def on_success(res): 
        self.d.callback(res)
      def on_error(res): 
        seld.d.errback(res)
      list_op_d.addCallback(on_success)
      list_op_d.addErrback(on_error) 

# class to get all log entries and writes them
# in JSON to a request object.
# the dict 'filter' describes how these results 
# can be filtered before being returned (see below)
class process_list_op: 

  def __init__(self,uel,datatypes,datacache):
    self.got = 0
    self.items = []
    self.all_spawned = False
    self.uel = uel
    self.datatypes = datatypes
    self.datacache = datacache

    self.name_to_dpid = {} 
    self.name_to_port = {} 
    self.unique_dpids = {} 

  def start(self, logids, filter): 
    self.d = Deferred()
    self.filter = filter 

    max = self.uel.get_max_logid()
    if max == 0: 
      self.done() 
      return self.d 
      
    # if nothing was provided, return ALL entries
    if logids is None: 
        min = self.uel.get_min_logid()
        logids = range(min,max+1)
   
    self.needs = 0
    for id in logids:
      if id > 0 and id <= max and id > filter["after"]:
        self.needs += 1
        self.uel.get_log_entry(id,self.log_callback) 
    # needed for common case when we call self.done() from self.log_callback()
    self.all_spawned = True
      
    if self.needs == self.got : 
      self.done() # nothing actually spawned, or everything already done

    return self.d

    
  def done(self):
    filtered_list = filter_item_list(self.items, ["app","msg"], 
                                          self.filter)
    ret_list = sort_and_slice_results(self.filter, filtered_list)
    self.d.callback(ret_list)

  def log_callback(self, logid, ts, app, level, msg, src_names, dst_names):
      self.got += 1
      if level != LogLevel.INVALID and level <= self.filter["max_level"]:
        e = make_entry(self.uel, self.datatypes, self.datacache,  
                      logid,ts,app,level,msg,src_names,dst_names)
        self.items.append(e) 

      if self.all_spawned and self.needs == self.got:
        self.done() 

class networkeventsws(Component): 
  """ Web service for network events (aka user_event_log)"""

  def __init__(self,ctx):
    Component.__init__(self,ctx)
    
  def getInterface(self):
    return str(networkeventsws)

  
  # this is mainly for debugging, though in the future it could be
  # a way for remote apps to integrate logging into our system. 
  def handle_add(self,request,data):
    try:
      if authorization_failed(request, [set(["add-network-events"])]):
        return NOT_DONE_YET
      content = json_parse_message_body(request)
      if content == None:
        content = {} 
      app = "via-netevent-webservice"
      if "app" in content: 
        app = str(content["app"])
      msg = "default webservice message"
      if "msg" in content: 
        msg = str(content["msg"])
      level = LogEntry.INFO
      if "level" in content: 
        level = int(content["level"])
      self.uel.log(app,level, msg)
    except Exception, e:
      err(Failure(), request, "handle_add",
          "Could not add log message")
    request.write(simplejson.dumps("success"))
    request.finish()
    return NOT_DONE_YET

  # this is mainly for debugging. 
  def handle_remove(self,request,data):
    if authorization_failed(request, [set(["remove-network-events"])]):
      return NOT_DONE_YET
    try:
      msg = ""
      def cb():
        try:
          request.write(simplejson.dumps("success:" + msg))
          request.finish()
        except Exception, e:
          err(Failure(), request, "handle_remove",
              "Could not remove log messages.")
    
      if(request.args.has_key("max_logid")): 
        max_logid = int(request.args["max_logid"][0])
        msg = "cleared entries with logid <= " + str(max_logid)
        self.uel.remove(max_logid,cb)
      else : 
        msg = "cleared all entries" 
        self.uel.clear(cb)
    except Exception, e:
      err(Failure(), request, "handle_remove",
          "Could not remove log messages.")
    return NOT_DONE_YET

  # returns a deferred that is called with the list of all log entries
  # for principal with name 'name' and type 'name_type', filtered by 
  # 'filter'.  If filter is not specified, all matching entries are returned
  def get_logs_for_name(self,uid,principal_type, filter=None): 
        if filter is None: 
          filter = self.get_default_filter() 
        p = process_name_op(self.uel, self.datatypes,self.datacache)
        return p.start(uid,principal_type,filter)
       
  # returns all logs if logid_list is None, or only the logs with logids
  # specified in 'logid_list'.  These results will be filtered if 'filter'
  # is specified.  
  def get_logs(self, logid_list = None, filter=None): 
      if filter is None: 
          filter = self.get_default_filter() 
      p = process_list_op(self.uel,self.datatypes,self.datacache)
      return  p.start(logid_list, filter)
  
  def get_default_filter(self): 
      return parse_mandatory_args({}, self.get_default_filter_arr())

  def get_default_filter_arr(self): 
      filter_arr = get_default_filter_arr("logid")
      filter_arr.extend([("after",0), ("max_level",LogLevel.INFO)])
      return filter_arr

  def handle_get_all(self,request,data):
    try : 
      if authorization_failed(request, [set(["get-network-events"])]):
        return NOT_DONE_YET
      filter = parse_mandatory_args(request,
          self.get_default_filter_arr())
      for s in ["app","msg"]:
        if s in request.args: 
          filter[s] = request.args[s][-1]

      # handles all requests that are filtering based on a particular 
      # principal name (e.g., host=sepl_directory;bob ) 
      type_map = {  "host" : self.datatypes.HOST, 
                    "user" : self.datatypes.USER, 
                    "location" : self.datatypes.LOCATION, 
                    "switch" : self.datatypes.SWITCH,  
                    "group" : self.datatypes.GROUP
                 } 
      for name, type in type_map.iteritems(): 
        if(request.args.has_key(name)):
          uid = int(request.args[name][0])
          d = self.get_logs_for_name(uid,type,filter)
          d.addCallback(dump_list_to_json, request)
          d.addErrback(err, request, "get_all",
            "Could not retrieve log messages")
          return NOT_DONE_YET

      # otherwise, we just query directory for logids
      # we query either for a single logid or for all
      logid_list = None  # default to query for all 
      if(request.args.has_key("logid")): 
        logid = int(request.args["logid"][0])
        max = self.uel.get_max_logid()
        if logid >= 1 and logid <= max: 
          logid_list = (logid)   
        else: 
          logid_list = () 
   
      d = self.get_logs(logid_list, filter) 
      d.addCallback(dump_list_to_json, request)
      d.addErrback(err, request, "get_all",
          "Could not retrieve log messages")
    except Exception, e: 
      err(Failure(), request, "get_all",
          "Could not retrieve log messages.")

    return NOT_DONE_YET

  def install(self):
    rwRoles = set(["Superuser", "Admin", "Demo"])
    roRoles = rwRoles | set(["Readonly"])
    webauth.Capabilities.register('get-network-events',
        'Get network event log messages', roRoles)
    webauth.Capabilities.register('add-network-events',
        'Add network event log messages', rwRoles)
    webauth.Capabilities.register('remove-network-events',
        'Remove network event log messages', rwRoles)

    self.uel = self.resolve(pyuser_event_log) 
    self.datatypes = self.resolve(Datatypes) 
    self.datacache = self.resolve(DataCache) 

    ws = self.resolve(webservice)
    v1 = ws.get_version("1") 
    
    # returns a JSON object: 
    #
    # { 'identifier' : 'logid' , items : [ .... ] } 
    #
    # Query Params:
    # * supports standard 'start' 'count' for pagination
    # * supports 'sort_descending' and 
    get_all_path = ( WSPathStaticString("networkevents"),)  
    v1.register_request(self.handle_get_all, "GET", get_all_path, 
                    """Get a set of messages from the network events log""")

    remove_path = (  WSPathStaticString("networkevents"), 
                      WSPathStaticString("remove"))  
    v1.register_request(self.handle_remove, "PUT", remove_path, 
                    """Permanently remove all (or just some) network event log entries""")

    add_path = (  WSPathStaticString("networkevents"), 
                      WSPathStaticString("add"))  
    v1.register_request(self.handle_add, "PUT", add_path, 
                    """Add a simple network event log message""")



def getFactory():
  class Factory:
    def instance(self,ctx):
      return networkeventsws(ctx)
  return Factory() 


