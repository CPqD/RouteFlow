#!/usr/bin/python
import os
import os.path
import sys
import getopt
import commands
import simplejson
## \ingroup utility
# @class nox_new_c_app
# nox-new-c-app.py utility creates a new C/C++ component in NOX.
# It is to be run in coreapps, netapps, webapps or one of their subdirectory. 
# Additional description is found in \ref new-c-component.
#
# Run nox-new-c-app.py --help for usage guide.
#
# @author ykk
# @date December 2009

##Run command
def run(cmd, dryrun=False, verbose=False):
    if (verbose):
        print cmd
    if (not dryrun):
        print commands.getoutput(cmd)

##Check file exists
def check_file(filename):
    if (not os.path.isfile(filename)):
        print filename+" not found!"
        sys.exit(2)

##Print usage guide
def usage():
    """Display usage
    """
    print "Usage "+sys.argv[0]+" <options> application_name"
    print "\tTo be run in coreapps, netapps or webapps of NOX source"
    print  "Options:"
    print "-h/--help\n\tPrint this usage guide"
    print "-d/--dry-run\n\tDry run"
    print "-v/--verbose\n\tVerbose"

#Parse options and arguments
##Dry-run or not
dryrun=False
##Verbose or not
verbose=False
try:
    opts, args = getopt.getopt(sys.argv[1:], "hdv",
                               ["help","dry-run","verbose"])
except getopt.GetoptError:
    print "Option error!"
    usage()
    sys.exit(2)

#Check there is only 1 application name
if not (len(args) == 1):
    print "Missing application name!"
    usage()
    sys.exit(2)
else:
    appname = args[0]

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
currdir=os.getcwd()
noxdir=currdir[currdir.find("src/nox/"):]
appdirname=None
appcategory=noxdir[(noxdir.rfind("/")+1):]
newdir=False
if (not (appcategory in ("coreapps","netapps","webapps"))):
    appdirname = appcategory
    noxdir = noxdir[:-(len(appdirname)+1)]
    appcategory = noxdir[(noxdir.rfind("/")+1):]
    if (not (appcategory in ("coreapps","netapps","webapps"))):
        print "This script adds a new application in NOX and"
        print "has to be run in coreapps, netapps or webapps"
        print "or one of their subdirectories accordingly."
        print "You can currently in invalid directory "+currdir
        sys.exit(2)
    else:
        print "Creating new application"
else:
    newdir=True
    print "Create new application and directory"

#Check sed
sedcmd=commands.getoutput("which sed")
if (sedcmd == ""):
    print "sed not found."
    sys.exit(2)

#Check configure.ac.in
configfile=None
if (newdir):
    configfile="../../../configure.ac.in"
    check_file(configfile)

#Check sample file
samplefile="simple_cc_app"
sampledir="../coreapps/simple_c_app/"
if (appdirname != None):
    sampledir="../"+sampledir
check_file(sampledir+samplefile+".hh")
check_file(sampledir+samplefile+".cc")
check_file(sampledir+"meta.json")
check_file(sampledir+"Makefile.am")

#Check application not existing
appname=appname.replace(" ","_")
if (newdir):
    appdirname=appname
    if (os.path.isdir(appdirname)):
        print appdirname+" already existing!"
        sys.exit(2)
else:
    if (os.path.isfile(appname+".cc") or
        os.path.isfile(appname+".hh")):
        print appname+" already existing!"
        sys.exit(2)

#Create application
if (newdir):
    run("mkdir "+appdirname, dryrun, verbose)
    #meta.json
    run("sed -e 's:"+samplefile+":"+appname+":g'"+\
        " -e 's:"+samplefile.replace("_"," ")+":"+\
        appname.replace("_"," ")+":g'"+\
        " < "+sampledir+"meta.json"+\
        " > "+appdirname+"/meta.json",
        dryrun, verbose)
    #Makefile.am
    run("sed -e 's:"+samplefile+":"+appdirname+":g'"+\
        " -e 's:coreapps:"+appcategory+":g'"+\
        " < "+sampledir+"Makefile.am"+\
        " > "+appdirname+"/Makefile.am",
        dryrun, verbose)
    run("sed -e 's:#add "+appcategory+" component here:"+\
        appdirname+" #add "+appcategory+" component here:g'"+\
        " < "+configfile+\
        " > "+configfile+".tmp",
        dryrun, verbose)
    run("mv "+configfile+".tmp "+configfile,
        dryrun, verbose)
    #C and Header file
    run("sed -e 's:"+samplefile+":"+appname+":g'"+\
        " < "+sampledir+samplefile+".hh"+\
        " > "+appdirname+"/"+appname+".hh",
        dryrun, verbose)
    run("sed -e 's:"+samplefile+":"+appname+":g'"+\
        " < "+sampledir+samplefile+".cc"+\
        " > "+appdirname+"/"+appname+".cc",
        dryrun, verbose)
else:
    #meta.json
    fileRef = open("meta.json","r")
    metafile = ""
    for line in fileRef:
        metafile += line
    fileRef.close()
    metainfo = simplejson.loads(metafile)
    appinfo = {}
    appinfo["name"] = appname.replace("_"," ")
    appinfo["library"] = appname
    metainfo["components"].append(appinfo)
    fileRef = open("meta.json","w")
    fileRef.write(simplejson.dumps(metainfo, indent=4))
    fileRef.close()
    #Makefile.am
    appMake = commands.getoutput("grep "+samplefile+"_la "+sampledir+"Makefile.am")\
              .replace("\n","\\n").replace(samplefile, appname)\
              .replace("coreapps",appcategory)+"\\n\\n"
    run("sed -e 's:pkglib_LTLIBRARIES =\\\\:pkglib_LTLIBRARIES =\\\\\\n\\t"+\
        appname+".la \\\\:g'"+\
        " -e 's:NOX_RUNTIMEFILES =:"+appMake+"NOX_RUNTIMEFILES =:g' "+\
        " < Makefile.am"+\
        " > Makefile.am.tmp",
        dryrun, verbose)
    run("mv Makefile.am.tmp Makefile.am",dryrun, verbose)
    #C and Header file
    run("sed -e 's:"+samplefile+":"+appname+":g'"+\
        " < "+sampledir+samplefile+".hh"+\
        " > "+appname+".hh",
        dryrun, verbose)
    run("sed -e 's:"+samplefile+":"+appname+":g'"+\
        " < "+sampledir+samplefile+".cc"+\
        " > "+appname+".cc",
        dryrun, verbose)

if (newdir):
    print "Please rerun ./boot.sh and ./configure"
