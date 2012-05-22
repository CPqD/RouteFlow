#!/usr/bin/python
import os
import os.path
import sys
import getopt
import simplejson
import re

## \ingroup utility
# @class nox_events
# Check events in NOX.
#
# This can only be run in the source directory of NOX
# This script does not parse Python register and so on.
#
# Run nox-events --help for help on usage
# 
# @author ykk
# @data June 2010
#

def readfile(filename):
    """Read file
    """
    fileRef = open(filename, "r")
    content = ""
    for line in fileRef:
        content += line
    fileRef.close()
    return content

class event:
    """Events in NOX
    """
    def __init__(self, name):
        """Initialize with name
        """
        self.name = name
        self.owner = None
        self.callers = []

    def __str__(self):
        """String representation
        """
        return self.name+"("+str(self.owner)+") called by\n\t"+str(self.callers)

class codeFile:
    """File with C code
    """
    def __init__(self, filename):
        """Initialize with content of file
        """
        self.content = []
        fileRef = open(filename, "r")
        for line in fileRef:
            self.content.append(line)
        fileRef.close()
        self.filename = filename
        self.__remove_comments()
        self.content = " ".join(self.content)
        self.events = self.parse_events()
        self.registering = self.parse_register()
        self.handling = self.parse_handler()

    def __remove_comments(self):
        """Remove all comments
        """
        fileStr = "".join(self.content)
        pattern = re.compile("\\\.*?\n", re.MULTILINE)
        fileStr = pattern.sub("",fileStr)
        pattern = re.compile(r"/\*.*?\*/", re.MULTILINE|re.DOTALL)
        fileStr = pattern.sub("",fileStr)
        pattern = re.compile("//.*$", re.MULTILINE)
        fileStr = pattern.sub("",fileStr)
        self.content = fileStr.split('\n')

    def parse_handler(self):
        """Parse file for event handled
        """
        handle = []
        pattern = re.compile("register_handler<.*?>.*?this", re.MULTILINE)
        for match in pattern.findall(self.content):
            handle.append((match[match.rfind("bind")+6:match.rfind("::")],
                           match[match.find("<")+1:match.find(">")]))
        return handle
        
    def parse_register(self):
        """Parse file for event registration
        """
        reg = []
        pattern = re.compile("void.*?configure.*?register_event\(.*?static_get_name", re.MULTILINE)
        for match in pattern.findall(self.content):
            match =  match[match.rfind("void"):]
            if (match.find("::") != match.rfind("::")):
                reg.append((match[4:match.find("::")].strip(),
                           match[(match.rfind("(")+1):match.rfind("::")]))
        return reg
    
    def parse_events(self):
        """Parse file for events
        """
        events = []
        pattern = re.compile("struct.*?:.*?Event"+
                             ".*?const.*?Event_name.*?static_get_name"+
                             ".*?return.*?\".*?\"", re.MULTILINE)
        for match in pattern.findall(self.content):
            events.append(match[match[:-1].rfind("\""):].replace("\"",""))
        return events

##Print usage guide
def usage():
    """Display usage
    """
    print "Usage "+sys.argv[0]+" <options> command"
    print "\tTo be run in src directory of NOX source"
    print  "Options:"
    print "-h/--help\n\tPrint this usage guide"
    print "-v/--verbose\n\tVerbose"
    print  "Commands:"
    print "list\n\tList all C/C++ events"
    print "check\n\tCheck nox.json"

#Parse options and arguments
##Verbose or not
verbose=False
try:
    opts, args = getopt.getopt(sys.argv[1:], "hv",
                               ["help","verbose"])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

#Check for commands
if not (len(args) >= 1):
    print "Missing command!"
    usage()
    sys.exit(2)
else:
    command = args[0]

#Parse options
for opt,arg in opts:
    if (opt in ("-h","--help")):
        usage()
        sys.exit(0)
    elif (opt in ("-v","--verbose")):
        verbose=True
    elif (opt in ("-d","--dry-run")):
        dryrun=True
    else:
        print "Unhandled option :"+opt
        sys.exit(2)

#Check current directory
if (not (os.path.isdir("nox/coreapps") and
         os.path.isdir("nox/netapps") and
         os.path.isdir("nox/webapps") and
         os.path.isfile("etc/nox.json"))):
    print "Error: Not in NOX's source directory"
    sys.exit(2)

#Find all events
cmd='find . -print'
register = []
handle = []
events = []
print "Parsing files..."
for hcfile in os.popen(cmd).readlines():
    hcfile = hcfile.strip()
    if (hcfile[-3:] == ".hh" or
        hcfile[-3:] == ".cc"):
        cFile = codeFile(hcfile)
        handle.extend(cFile.handling)
        events.extend(cFile.events)
        register.extend(cFile.registering)
        
print "Getting library to name mapping..."
lib2name = {}
cmd='find . -name "meta.json" -print'
for metafile in os.popen(cmd).readlines():
    meta = simplejson.loads(readfile(metafile.strip()))
    for component in meta["components"]:
        try:
            lib2name[component["library"].lower()] = component["name"]
        except KeyError:
            pass

print "Creating event database"
#Generate events
allevents = {}
for e in events:
    allevents[e] = event(e)
for h in handle:
    try:
        allevents[h[1]]
    except KeyError:
        allevents[h[1]] = event(h[1])
#Find registering and handling    
for r in register:
    try:
        allevents[r[1]].owner = lib2name[r[0].lower()]
    except KeyError:
        pass
for h in handle:
    try:
        allevents[h[1]].callers.append(lib2name[h[0].lower()])
    except KeyError:
        pass
print

#Process commands
if (command == "list"):
    #List events
    for (ae, aeo) in allevents.items():
        if (verbose):
            print str(aeo)
        else:
            print ae
elif (command == "check"):
    #Check nox.json
    noxjson = simplejson.loads(readfile("etc/nox.json"))
    for (ae, aeo) in allevents.items():
        if (len(aeo.callers) != 0):
            #Check is listed
            try:
                noxjson["nox"]["events"][aeo.name]
                #Check callers are listed
                for caller in aeo.callers:
                    if (not (caller in noxjson["nox"]["events"][aeo.name])):
                        print caller+" calls "+aeo.name+" but not registered"
                        if (verbose):
                            print "\t"+str(noxjson["nox"]["events"][aeo.name])+" are registered"
                            print
            except KeyError:
                print str(aeo.name)+" with "+str(len(aeo.callers))+\
                      " callers is not registered in nox.json"
                if (verbose):
                    print "\tcalled by "+str(aeo.callers)
                    print        
else:
    print "Unknown command: "+command

