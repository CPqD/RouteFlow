#!/usr/bin/python
import os
import sys
import getopt
import socket
import simplejson

## \ingroup utility
# @class stats_client
# Sends a JSON command to NOX.
# Uses \ref jsonmessenger to proxy commands.
# Individual components will execute the commands accordingly.
# Based on NOX's script "src/scripts/nox-console.py"
#
# Run stats_client --help for help on usage
#
# @author ykk
# @author joestringer
# @data February 2012

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
    print  "Commands:"
    print "features\n\tDo features-request on jsonstats server"
    print "ports\n\tDo port_stats-request on jsonstats server"

try:
    opts, args = getopt.getopt(sys.argv[1:], "hvn:p:r",
                               ["help","verbose","noxhost=","port=",
                                "noreply"])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

nox_host = "localhost"
port_no = 2703

# Parse our commandline args
for opt,arg in opts:
    if (opt in ("-h","--help")):
        usage()
        sys.exit(0)
    elif (opt in ("-n","--noxhost")):
        nox_host=arg
    elif (opt in ("-p","--port")):
        port_no=int(arg)
    else:
        print "Unhandled option :"+opt
        sys.exit(2)

cmd = {}
if (args[0] == "features"):
    cmd["type"] = "jsonstats"
    cmd["command"] = "features_request"
elif (args[0] == "ports"):
    cmd["type"] = "jsonstats"
    cmd["command"] = "stats_request"
else:
    print "Unknown command: "+args[0]
    sys.exit(2)

# Connect, send request, receive reply
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((nox_host,port_no))

sock.send(simplejson.dumps(cmd))
reply = simplejson.dumps(simplejson.loads(sock.recv(4096)), indent=4)

sock.send("{\"type\":\"disconnect\"}")
sock.shutdown(1)
sock.close()

print reply
print "STATS_CLIENT: bytes received: " + str(len(reply))
