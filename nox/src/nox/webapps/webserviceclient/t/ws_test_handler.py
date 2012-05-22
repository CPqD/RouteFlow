#!/usr/bin/env python
import sys
from optparse import OptionParser
import sys
import httplib
import os
import simplejson
import sys
import urllib
import traceback

#returns the modified command and cmdargs wherever applicable
#def handle_group_cmd(req,wsc,cmd , cmdarg):
#       uid = get_uid(wsc,cmd.replace("/uid",""),cmdarg)
#      cmd = cmd.replace("uid","%d" %uid)
#       return cmd , cmdarg


#def get_uid(wsc,cmd,cmdarg):
#       uid = 0 
#      #get all records ,find the uid
#       response = wsc.get(urllib.quote(cmd))
#       rec_list = simplejson.loads(response.getBody())
#       for rec in rec_list:
#         print rec
#         if rec[1] == cmdarg["find-name"]:
#           uid = int(rec[0])
#           break
#       print "UID %d" %uid
#       assert uid != 0 , " %s does not exist" % cmdarg["find-name"]
#      return uid
