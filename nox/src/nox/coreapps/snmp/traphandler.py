#!/usr/bin/python
import sys
import simplejson
import getopt
import socket
## 
# @class traphandler
# Parse SNMP trap messages and send to NOX via jsonmessenger.
# 
# @see snmp
# @see vigil::jsonmessenger
# @author ykk
# @date June 2010

##Print usage guide
def usage():
    """Display usage
    """
    print "Usage: to be used in snmptrapd.conf"
    print "\tTo send SNMP trap message to NOX"
    print  "Options:"
    print "-h/--help\n\tShow this usage guide"
    print "-n/--noxhost\n\tSpecify where NOX is hosted (default:localhost)"
    print "-p/--port\n\tSpecify port number for messenger (default:2703)"
    print "-v/--verbose\n\tVerbose mode"

#Parse options and arguments
try:
    opts, args = getopt.getopt(sys.argv[1:], "hvn:p:",
                               ["help","verbose","noxhost=","port="])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

#Parse options
##NOX host
nox_host = "localhost"
##Port number
port_no = 2703
#Debug
debug = False
for opt,arg in opts:
    if (opt in ("-h","--help")):
        usage()
        sys.exit(0)
    elif (opt in ("-n","--noxhost")):
        nox_host=arg
    elif (opt in ("-p","--port")):
        port_no=int(arg)
    elif (opt in ("-v","--verbose")):
        debug=True
    else:
        print "Unhandled option :"+opt
        sys.exit(2)

#Get data for SNMP trap/inform
data = []
cmd = {}
cmd["type"] = "snmptrap"
i = 0
for line in sys.stdin.readlines():
    if (i == 0):
        cmd["host"] = line.strip()
    elif (i == 1):
        cmd["ip"] = line.strip()
    else:
        linedata = line.split()
        if (len(linedata) == 2):
            cmd[linedata[0]] = linedata[1]
        elif (len(linedata) >= 2):
            cmd[linedata[0]] = linedata[1:]
        else:
            if (len(linedata) > 0):
                cmd[linedata[0]] = None
    i+=1

#Construct message

#Sending content
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((nox_host,port_no))
if (debug):
    fileRef = open("/tmp/snmptrapmsg","w")
    fileRef.write(simplejson.dumps(cmd))
    fileRef.close()
sock.send(simplejson.dumps(cmd))
sock.send("{\"type\":\"disconnect\"}")
sock.shutdown(1)
sock.close()
