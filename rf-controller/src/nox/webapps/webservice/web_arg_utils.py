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
import StringIO
import re
import sys
from nox.netapps.bindings_storage.bindings_directory import *

# This file contains utility functions that are likely to be useful 
# to implementing many different webservices.  


# convert a string that is intended to be a glob and 
# make it so we can safely use it to create a python regex.
# this is based a method in dojo:
# dojo-release-1.1.1/dojo/data/util/filter.js
def glob_to_regex(glob_str, ignore_case=True): 
  rxp = ""
  if ignore_case:
    rxp += "(?i)"
  rxp += "^"
  i = 0 
  while i < len(glob_str): 
    c = glob_str[i]
    if c == '\\':
        rxp += c
        i += 1
        rxp += glob_str[i]
    elif c == '*':
        rxp += ".*" 
    elif c ==  '?':
        rxp += "." 
    elif c == '$' or \
      c == '^' or \
      c == '/' or \
      c == '+' or \
      c == '.' or \
      c == '|' or \
      c == '(' or \
      c == ')' or \
      c == '{' or \
      c == '}' or \
      c == '[' or \
      c == ']' : 
        rxp += "\\"
        rxp += c
    else :
        rxp += c
    i += 1

  rxp += "$"
  return rxp 


# the 'args' dictionary of a json request
# object maps from strings to lists of strings.
# this flattens it to a map of strings to strings
# by just taking the first element of the list
# 'skip' indicates a list of keys that should not be copied
def flatten_args(args,skip = [] ): 
      new_args = {} 
      for key in args.keys():
        if not key in skip: 
          new_args[key] = args[key][0]
      return new_args

# returns a comparison function that can be passed to list's sort() method
# 'res_list' must contain at least one element
# assumes a list like:
# [ { attr1: "foo", attr2 : "bar" }, {attr1 : "bin", attr2 : "bash" } ]
def new_get_cmp_fn(res_list, attr_name, sort_descend ):

  if not attr_name in res_list[0]: 
    raise Exception("invalid 'sort_attribute'. dict " \
        "has no key = '%s'" % attr_name)
  
    # if specified attribute is an int, we do int comparsion
  # otherwise we do the standard string comparison 
  attr_is_int = isinstance(res_list[0][attr_name],int)

  max = sys.maxint 
  min = -sys.maxint - 1
  def descend_int(a,b):
      a_val = a[attr_name]
      b_val = b[attr_name]
      if a_val is None: return max
      if b_val is None: return min
      return b_val - a_val
  def descend_default(a,b):
      return cmp(b[attr_name],a[attr_name])
  def ascend_int(a,b):
      a_val = a[attr_name]
      b_val = b[attr_name]
      if a_val is None: return min
      if b_val is None: return max
      return a_val - b_val
  def ascend_default(a,b):
      return cmp(a[attr_name],b[attr_name])

  if sort_descend:
    if attr_is_int: 
      return descend_int
    else: 
      return descend_default
  else :  # ascending 
    if attr_is_int: 
      return ascend_int
    else: 
      return ascend_default

#based on the contents of the 'filter' dict, this function
# returns a comparison function that can be passed to list's sort()
# method
def get_cmp_fn(filter,res_list):
    sort_descend = filter["sort_descending"] == "true"

    # no field to sort on was specified in the URL and
    # no default specified by the webservice.  Just do normal cmp
    if not filter["sort_attribute"]: 
      if sort_descend: 
        return lambda a, b : cmp(b,a)
      else: 
        return lambda a, b : cmp(a,b)
 
    # test that sort attribute is a key in the dict
    attr_name = filter["sort_attribute"]
    if not attr_name in res_list[0]: 
      raise Exception("invalid 'sort_attribute'. dict " \
         "has no key = '%s'" % attr_name)
   
    # if specified attribute is an int, we do int comparsion
    # otherwise we do the standard string comparison 
    attr_is_int = isinstance(res_list[0][attr_name],int)
 
    if sort_descend:
      if attr_is_int: 
        return lambda a,b: b[attr_name] - a[attr_name]
      else: 
        return lambda a,b: cmp(b[attr_name],a[attr_name])
    else :  # ascending 
      if attr_is_int:       
        return lambda a,b: a[attr_name] - b[attr_name]
      else:
        return lambda a,b: cmp(a[attr_name],b[attr_name])


# sorts and slices a list of elements based on the
# content of the 'filter' dict.  
def sort_and_slice_results(filter,res_list):
  if(len(res_list) > 1): 
    entry_cmp = get_cmp_fn(filter,res_list)   
    res_list.sort(cmp=entry_cmp)
    
  if "count" not in filter or filter["count"] == -1 : 
    filter["count"] = len(res_list) 
  if "start" not in filter: 
    filter["start"] = 0

  slice_start = filter["start"]
  slice_end = slice_start + filter["count"]
  return res_list[slice_start : slice_end]

# standard defintion of the params a webservice can use to 
# sort and filter results
def get_default_filter_arr(default_sort_attr = ""): 
    return [ ("start",0),("count", -1), ("sort_attribute",default_sort_attr),
                      ("sort_descending","false")]

# parses arguments from a response into a dictionary.
# input is a list of (key,default) tuples, where key is a
# string and 'default' is of any type.  If the request 
# does not contain a parameter, it is set to the default value.
# The type of the 'default' value indicates how this method 
# will cast a value provided in the request.
def parse_mandatory_args(request, key_arr): 
    filter = {}
    for key,default in key_arr: 
      filter[key] = default
      try: 
        type_obj = type(default)
        filter[key] = type_obj(request.args[key][0]) 
      except: 
        pass # an exception here is normal
    return filter

def grab_boolean_arg(request, argname, default):    
        args = request.args
        if not args.has_key(argname):
            return default
        if len(args[argname]) != 1:
            return default
        if args[argname][0].lower() == 'true':
            return True
        if args[argname][0].lower() == 'false':
            return False 
        return default    

def grab_string_arg(request, argname):    
        args = request.args
        if not args.has_key(argname):
            return None
        if len(args[argname]) != 1:
            return None
        return args[argname][0]

def find_value_in_args(args,values): 
  for v in values: 
    if v in args: return v
  raise Exception("none of the these names exists in URL: %s" % values); 

def get_principal_type_from_args(args): 
    values = [ "switch","location","host","user","nwaddr","dladdr",
               "hostnetname" ]
    return find_value_in_args(args,values) 

def get_nametype_from_string(str): 
      if str == "host" or str == "hosts": return Name.HOST
      if str == "user" or str == "users" : return Name.USER
      if str == "location" or str == "locations" : return Name.LOCATION
      if str == "switch" or str == "switches" : return Name.SWITCH
      if str == "port" or str == "ports" : return Name.PORT
      raise Exception("%s is not a valid name type" % str)


# filters a list of items based on the contents of a web
# request's args.
# item_list : a list of string attribute-to-string value dictionaries that 
#             represent data rows needing to be filtered 
# attr_list : the list of attribute names that, if present as keys
#             in 'args', will be used to filter elements of 'item_list' 
#             how URL parameters from the webrequest should be used
#             to filter attributes in each item.
# filter_map  : map from attribute keys to the value that will be used as a
#             glob filter for that attribute's data column. 
def filter_item_list(item_list, attr_list, filter_map): 
  live_attrs = [] 
  regex_map = {} 
  for attr in attr_list:
    if attr in filter_map:
        live_attrs.append(attr)
        r = re.compile(glob_to_regex(filter_map[attr]),re.IGNORECASE)
        regex_map[attr] = r

  if len(live_attrs) == 0: 
    return item_list # nothing to filter

  new_item_list = [] 
  for item in item_list: 
    matches = True
    for a in live_attrs:
      r = regex_map[a]
      if not r.match(item[a]):
        matches = False
        break
    if matches: 
      new_item_list.append(item) 
        
  return new_item_list
 
def get_html_for_select_box(options, selected_val,select_node_attrs): 
  buf = StringIO.StringIO()
  buf.write('<select')
  for a in select_node_attrs: 
    buf.write(' %s="%s" ' % a)
  buf.write('>')
  for v in options:
    if v == selected_val: 
      buf.write('<option value="%s" selected="true"> %s </option>' % (v,v))
    else: 
      buf.write('<option value="%s"> %s </option>' % (v,v))
  buf.write('</select>') 
  str = buf.getvalue()
  buf.close() 
  return str
