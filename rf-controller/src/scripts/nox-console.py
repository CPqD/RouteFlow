#!/usr/bin/python
import os
import sys
import getopt
import socket
import simplejson

## \ingroup utility
# @class nox_console
# Sends a JSON command to NOX.
# Uses \ref jsonmessenger to proxy commands.
# Individual components will execute the commands accordingly.
#
# Run nox-console --help for help on usage
# 
# @author ykk
# @data June 2010
#

##Print usage guide
def usage():
    """Display usage
    """
    print "Usage "+sys.argv[0]+" <options> command <parameters>"
    print "\tTo send JSON command to NOX"
    print  "Options:"
    print "-h/--help\n\tPrint this usage guide"
    print "-n/--noxhost\n\tSpecify where NOX is hosted (default:localhost)"
    print "-p/--port\n\tSpecify port number for messenger (default:2703)"
    print "-r/--no-reply\n\tSpecify if reply is expected (default:True)"
    print "-v/--verbose\n\tVerbose output"
    print  "Commands:"
    print "getnodes [node_type]\n\tGet nodes (of type if specified)"
    print "getlinks [node_type]\n\tGet links (of type if specified)"

#Parse options and arguments
try:
    opts, args = getopt.getopt(sys.argv[1:], "hvn:p:r",
                               ["help","verbose","noxhost=","port=",
                                "noreply"])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

#Parse options
##Verbose debug output or not
debug = False
##NOX host
nox_host = "localhost"
##Port number
port_no = 2703
##Wait for reply
expectReply = True
for opt,arg in opts:
    if (opt in ("-h","--help")):
        usage()
        sys.exit(0)
    elif (opt in ("-v","--verbose")):
        debug=True
    elif (opt in ("-n","--noxhost")):
        nox_host=arg
    elif (opt in ("-r","--no-reply")):
        expectReply = False
    elif (opt in ("-p","--port")):
        port_no=int(arg)
    else:
        print "Unhandled option :"+opt
        sys.exit(2)

if not (len(args) >= 1):
    print "Missing command!"
    usage()
    sys.exit(2)

#Construct command
cmd = {}
if (args[0] == "getnodes"):
    cmd["type"] = "lavi"
    cmd["command"] = "request"
    if (len(args) == 1):
        cmd["node_type"] = "all"
    else:
        cmd["node_type"] = args[1]
elif (args[0] == "getlinks"):
    cmd["type"] = "lavi"
    cmd["command"] = "request"
    if (len(args) == 1):
        cmd["link_type"] = "all"
    else:
        cmd["link_type"] = args[1]
else:
    print "Unknown command: "+args[0]
    sys.exit(2)

#Send command
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((nox_host,port_no))
if (debug):
    print cmd
    print simplejson.dumps(cmd)
sock.send(simplejson.dumps(cmd))
if (expectReply):
    print simplejson.dumps(simplejson.loads(sock.recv(4096)), indent=4)
sock.send("{\"type\":\"disconnect\"}")
sock.shutdown(1)
sock.close()
