#!/usr/bin/python
import os
import sys
import getopt
import nox.coreapps.messenger.messenger as msg
## \ingroup utility
# @class nox_send_cmd
# Sends a string command to NOX.
# Uses \ref messenger to proxy commands.
# Individual components will execute the commands accordingly.
#
# This can only be run in the source directory of NOX
# unless that directory is added to the PYTHONPATH
# environment variable.
#
# Run nox-send-cmd.py --help for help on usage
# 
# @author ykk
# @data January, 2010
#

##Print usage guide
def usage():
    """Display usage
    """
    print "Usage "+sys.argv[0]+" <options> string_command"
    print "\tTo send command to NOX"
    print  "Options:"
    print "-h/--help\n\tPrint this usage guide"
    print "-n/--noxhost\n\tSpecify where NOX is hosted (default:localhost)"
    print "-p/--port\n\tSpecify port number for messenger (default:2603)"
    print "-v/--verbose\n\tVerbose output"

#Parse options and arguments
try:
    opts, args = getopt.getopt(sys.argv[1:], "hvn:p:",
                               ["help","verbose","noxhost=","port="])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

if not (len(args) >= 1):
    print "Missing command!"
    usage()
    sys.exit(2)
else:
    str_cmd = " ".join(args)

#Parse options
##Verbose debug output or not
debug = False
##NOX host
nox_host = "localhost"
##Port number
port_no = 2603
##NOX command type
nox_cmd_type = 10
for opt,arg in opts:
    if (opt in ("-h","--help")):
        usage()
        sys.exit(0)
    elif (opt in ("-v","--verbose")):
        debug=True
    elif (opt in ("-n","--noxhost")):
        nox_host=arg
    elif (opt in ("-p","--port")):
        port_no=int(arg)
    else:
        print "Unhandled option :"+opt
        sys.exit(2)

#Send command
channel = msg.channel(nox_host, port_no, debug)
cmdmsg = msg.messenger_msg()
cmdmsg.type = msg.MSG_NOX_STR_CMD
cmdmsg.length = len(cmdmsg)+len(str_cmd)
channel.send(cmdmsg.pack()+str_cmd)
