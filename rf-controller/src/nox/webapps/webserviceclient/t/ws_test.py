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

from twisted.python.failure import Failure

# Placing the empty string at the beginning of sys.path ensures
# it will look in the current directory first.
sys.path.insert(0, '')
import logging
log = logging.getLogger("ws-test")

from nox.webapps.webserviceclient.simple import NOXWSClient, PersistentLogin, \
            FailureExceptionLogin


DEFAULT_NOXWS_USER = "admin"
WS_VERSION = "ws.v1"


def read_input():
        """assumes input is provided in dictionary format
            coverts to dict format"""   
        data = ""
        for line in sys.stdin:
            data += line 
        #remove the new lines and convert to dictionary
        data =  data.replace("\n","") 
        return eval(data)

#returns the output as list of  key:value pairs
#key represents the depth  of the value.
#example data = [[19,100....]] ===> key= body[0][1][2] = 100
#example data = [{"uid": 10,...},{}} ==> key= body.uid = 10
def parse_output(key,value,list= []):
    if type(value).__name__ in ['int','str','unicode','NoneType','bool','float']:
         list.append((key,repr(value)))
    elif type(value).__name__ == 'list':
        cnt = 0
        for ele in value:
           parse_output("%s[%s]" %(key,cnt),ele,list)
           cnt = cnt+1
    elif type(value).__name__ == 'dict':
        for k,d in value.iteritems():
           k = k.replace(".","\.").replace("=","\=")
           parse_output("%s.%s" %(key,k),d,list)
    else:
        list.append((repr("unknown type"),repr(value)))
    return list


#print the response in key = value format
def print_response(response):
       print "status_code = '%s'" %response.status
       parsed_h = []
       parsed_msg = []
       headers = response.getHeaders()
       parsed_h = parse_output("header",headers,parsed_h)
       for k,d  in parsed_h:
            print "%s = %s" % (k, d)
       msg_body = simplejson.loads(response.getBody())
       parsed_msg = parse_output("body",msg_body,parsed_msg)
       parsed_msg.sort()
       for k,d in parsed_msg:
            print "%s = %s" %(k,d)


def response_is_valid(response):
        contentType = response.getContentType()
        print_response(response)
        #check the output to the status returned
        assert response.status == httplib.OK, \
            'Request error %d (%s) : %s' % \
            (response.status, response.reason, response.getBody())
        assert contentType == "application/json", \
            "Unexpected content type: %s : %s" % \
            (contentType, response.getBody())
        return True

def ws_cmdGet(wsc,cmd):
        """ Executes the Get WS calls """
        # For commands that require the uid invoke the command handler
        response = wsc.get(urllib.quote(cmd))
        response_is_valid(response)
      
def ws_cmdPost(wsc,cmd):
        """ Executes the Post WS calls """
        cmdarg = read_input()
        response = wsc.postAsJson(cmdarg,urllib.quote(cmd))
        response_is_valid(response)

def ws_cmdPut(wsc,cmd):
        """ Executes the Put WS calls """
        # For commands that require the uid invoke the command handler
        cmdarg = read_input()
        #for "Put command the uid is must field"
        cmdarg["uid"] =  cmd.split('/')[-1]
        response = wsc.putAsJson(cmdarg, urllib.quote(cmd))
        response_is_valid(response)
 

def ws_cmdDelete(wsc,cmd):
        """ Executes the Delete WS calls """
        #some commands need to be modified hence invoke command handler
        response = wsc.delete(urllib.quote(cmd))
        response_is_valid(response)
       

if __name__ == "__main__":
   #read comand line garguments
    optParser = OptionParser(usage="usage: %prog [options] [webservice cmd]")

    optParser.add_option("-H", "--host", dest="host",
                      type="string", default="127.0.0.1",
                      help="Host to connect to (default=localhost)")
    optParser.add_option("-u", "--user", dest="user",
                      type="string", default=DEFAULT_NOXWS_USER,
                      help="Username to provide for login (default=%default).")
    optParser.add_option("-p", "--password", dest="password",
                      type="string", default=DEFAULT_NOXWS_USER,
                      help="Password to provide for login (default=%default).")
    optParser.add_option("--ssl", dest="ssl",
                      action="store_true", default=True,
                      help="Use SSL. (default)")
    optParser.add_option("--no-ssl", dest="ssl",
                      action="store_false", default=True,
                      help="Do not use SSL.")
    optParser.add_option("-P", "--port", dest="port",
                      type="int", default=None,
                      help="Destination port number to connect to.")
    optParser.add_option("-r", "--req_method",dest="req_method",
                      type="string", default="Get",
                      help="Provide the request method: Get, Post, Put, Delete")
    optParser.add_option("", "--debug", dest="debug",
                      action="store_true", default=False,
                      help="Enable debug logging")
    optParser.add_option("", "--debug-logfile", dest="debugLogfile",
                      type="string", default="./.ncli.debug.log",
                      help="Dest file for debug logging (default=%default).")


    (options, args) = optParser.parse_args()
    if options.port == None:
        if options.ssl:
            options.port = 443
        else:
            options.port = 8888
    if options.debug == True:
        if options.debugLogfile == "":
            logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
        else:
            logging.basicConfig(level=logging.DEBUG,
                                filename=options.debugLogfile)
    else:
        logging.basicConfig(level=logging.FATAL, stream=sys.stdout)

    loginmgr = PersistentLogin(options.user, options.password)
    wsc = NOXWSClient(options.host, options.port, options.ssl, loginmgr)
    
    print args[0]
      
    try:
        locals()["ws_cmd%s" %options.req_method](wsc,args[0])
    except Exception, e:
        traceback.print_exc(file=sys.stderr)
        print  >>sys.stderr ,  "Status: Failed"
        exit(0)

   

