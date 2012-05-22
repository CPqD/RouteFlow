/* Copyright 2008, 2009 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \mainpage
 *
 * \section overview Overview
 *
 * This is an automatically generated documentation for NOX.  Recent API are fairly
 * well-documented, though the documentation of olders API is pretty much hit 
 * and miss.  This serves well as a guide for individual components 
 * and detailed API calls.  You might want to refer to the wiki for tutorial and general
 * explanations.  The modules page (and pages therein) might be most interesting:
 *
 * - Some \ref noxapi "basic API" in NOX is documented here
 * - The existing \ref noxcomponents "components providing application functionality"
 * - The \ref noxevents "events core NOX and the components generate"
 * - A few \ref utility utilites distributed with NOX
 * - (Akan Datang)The \ref nox-programming-model "cooperative threading, asynchronous, event-based programming model"
 * 
 */

/** \page nox-programming-model The NOX Programming Model
 *
 * ... Write a nice intro here ...
 *
 * \section prog-model-threading Cooperative Threading
 *
 * \section prog-model-async Asynchronous Programming
 * \subsection prog-model-async-c C++
 * \subsection prog-model-async-python Python
 *
 * \section prog-model-events Event Programming
 * \subsection prog-model-events-using Using Existing Events
 * \subsection prog-model-events-creating Creating New Events
 *
 */


#include "config.h"

#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <inttypes.h>
#include <unistd.h>

#include <list>
#include <set>
#include <string>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <fstream>

#include "builtin/event-dispatcher-component.hh"
#include "bootstrap-complete.hh"
#include "command-line.hh"
#include "component.hh"
#include "dso-deployer.hh"
#include "fault.hh"
#include "kernel.hh"
#include "leak-checker.hh"
#include "openflow.hh"
#include "nox.hh"
#include "nox_main.hh"
#include "shutdown-event.hh"
#include "static-deployer.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"
#ifndef LOG4CXX_ENABLED
#include "vlog-socket.hh"
#else
#include "log4cxx/helpers/exception.h"
#include "log4cxx/xml/domconfigurator.h"
#endif
#include "json-util.hh"
#include "json_object.hh"

#include "openflow/openflow.h"

using namespace vigil;
using namespace vigil::main;
using namespace std;

namespace {

Vlog_module lg("nox");

typedef std::string Application_name;
typedef std::pair<Application_name, std::list<std::string> > Application_params;
typedef std::list<Application_params> Application_list;

Application_list
parse_application_list(int optind, int argc, char** argv) {
    Application_list applications;
    
    set<string> applications_defined;
    for (int i = optind; i < argc; i++) {
        vector<string> tokens;
        for (char* token = strtok(argv[i], "="); token != NULL;
             token = strtok(NULL, ",")) {
            tokens.push_back(token);
        }
        if (tokens.empty()) {
            lg.err("Empty argument on command line");
            continue;
        }
        
        string app = tokens.front();
        if (applications_defined.find(app) != applications_defined.end()) {
            lg.err("Application '%s' defined multiple times in the command line. "
                   "Ignoring the latter definition(s).", app.c_str());
            continue;
        }
        applications_defined.insert(app);
        
        list<string> args(tokens.begin() + 1, tokens.end());
        applications.push_back(make_pair(app, args));
    }
    
    return applications;
}

void hello(const char* program_name)
{
    printf("NOX %s (%s), compiled " __DATE__ " " __TIME__ "\n", NOX_VERSION, 
           program_name);
    printf("Compiled with OpenFlow 0x%x%x %s\n", 
           (OFP_VERSION >> 4) & 0x0f, OFP_VERSION & 0x0f,
           (OFP_VERSION & 0x80) ? "(exp)" : "");
}

void usage(const char* program_name)
{
    printf("%s: nox runtime\n"
	   "usage: %s [OPTIONS] [APP[=ARG[,ARG]...]] [APP[=ARG[,ARG]...]]...\n"
           "\nInterface options (specify any number):\n"
#ifdef HAVE_NETLINK
           "  -i nl:DP_ID             via netlink to local datapath DP_IDX\n"
#endif
           "  -i ptcp:[PORT]          listen to TCP PORT (default: %d)\n"
           "  -i pssl:[PORT]:KEY:CERT:CONTROLLER_CA_CERT\n"        
           "                          listen to SSL PORT (default: %d)\n"
           "  -i pcap:FILE[:OUTFILE]  via pcap from FILE (for testing) write to OUTFILE\n"
           "  -i pcapt:FILE[:OUTFILE] same as \"pcap\", but delay packets based on pcap timestamps\n"
           "  -i pgen:                continuously generate packet-in events\n"
           "\nNetwork control options (must also specify an interface):\n"
           "  -u, --unreliable        do not reconnect to interfaces on error\n",
	   program_name, program_name, OFP_TCP_PORT, OFP_SSL_PORT);
    leak_checker_usage();
    printf("\nOther options:\n"
           "  -c, --conf=FILE         set configuration file\n"
           "  -d, --daemon            become a daemon\n"
           "  -l, --libdir=DIRECTORY  add a directory to the search path for application libraries\n"
           "  -p, --pid=FILE          set pid file\n"
           "  -n, --info=FILE         set controller info file\n"
	   "  -v, --verbose           set maximum verbosity level (for console)\n"
#ifndef LOG4CXX_ENABLED
	   "  -v, --verbose=CONFIG    configure verbosity\n"
#endif
	   "  -h, --help              display this help message\n"
	   "  -V, --version           display version information\n");
    exit(EXIT_SUCCESS);
}

int daemon() {
    switch (fork()) {
    case -1:
        /* Fork failed! */
        return -1;

    case 0:
        /* Daemon process */
        break;

    default:
        /* Valid PID returned, exit the parent process. */
        exit(EXIT_SUCCESS);
        break;
    }
    
    /* Change the file mode mask */
    umask(0);
    
    /* Create a new SID for the child process */
    if (setsid() == -1) {
        return -1;
    }

    /* Close out the standard file descriptors */
    int fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }

    return 0;
}

int start_gui() {
    switch (fork()) {
    case -1:
        /* Fork failed! */
        return -1;
    case 0:
        /* Daemon process */
        char *args[] = {"", (char *) 0 };
        execv("../../src/gui/qt-nox.py", args);
        cout<<"Starting GUI\n";
        break;
    }
    return 0;
}


bool verbose = false;
#ifndef LOG4CXX_ENABLED
vector<string> verbosity;
#endif

void init_log() {
#ifndef LOG4CXX_ENABLED
    static Vlog_server_socket vlog_server(vlog());
    vlog_server.listen();
#endif

#ifdef LOG4CXX_ENABLED
    /* Initialize the log4cxx logging infrastructure and override the
       default log level defined in the configuration file with DEBUG
       if 'verbose' set.  */
    ::log4cxx::xml::DOMConfigurator::configureAndWatch("etc/log4cxx.xml");
    if (verbose) {
        ::log4cxx::Logger::getRootLogger()->
            setLevel(::log4cxx::Level::getDebug());
    }
#else
    if (verbose) {
        set_verbosity(0);
    }
#endif
}

} // unnamed namespace

Disposition remove_pid_file(const char* pid_file, const Event&) {
    unlink(pid_file);

    return CONTINUE;
}

static void finish_booting(Kernel*, const Application_list&, vector<string>, 
                           bool);

int main(int argc, char *argv[])
{
    /* Program name without full path or "lt-" prefix.  */
    const char *program_name = argv[0];
    if (const char *slash = strrchr(program_name, '/')) {
        program_name = slash + 1;
    }
    if (!strncmp(program_name, "lt-", 3)) {
        program_name += 3;
    }
    
    const char* pid_file = "/var/run/nox.pid";
    const char* info_file = "./nox.info";
    bool reliable = true;
    bool daemon_flag = false;
    bool gui_flag = false;
    vector<string> interfaces;

    string conf = PKGSYSCONFDIR"/nox.json";

    /* Where to look for configuration file.  Check local dir first,
       and then global install dir. */
    list<string> conf_dirs; 
    conf_dirs.push_back(PKGSYSCONFDIR"/nox.json");
    conf_dirs.push_back("etc/nox.json");

    /* Add PKGLIBDIRS and local build dir to libdirectory */
    list<string> lib_dirs;
    lib_dirs.push_back("nox/coreapps/");
    lib_dirs.push_back("nox/netapps/");
    lib_dirs.push_back("nox/webapps/");
    lib_dirs.push_back("nox/ext/");

    for (;;) {
        enum {
            OPT_CHECK_LEAKS = UCHAR_MAX + 1,
            OPT_LEAK_LIMIT
        };
        static struct option long_options[] = {
            {"daemon",      no_argument, 0, 'd'},
            {"gui",      no_argument, 0, 'g'},
            {"unreliable",  no_argument, 0, 'u'},

            {"interface",   required_argument, 0, 'i'},

            {"conf",        required_argument, 0, 'c'},
            {"libdir",      required_argument, 0, 'l'},
            {"pid",         required_argument, 0, 'p'},
            {"info",        required_argument, 0, 'n'},

            {"check-leaks", required_argument, 0, OPT_CHECK_LEAKS},
            {"leak-limit",  required_argument, 0, OPT_LEAK_LIMIT},

#ifdef LOG4CXX_ENABLED
            {"verbose",     no_argument, 0, 'v'},
#else
            {"verbose",     optional_argument, 0, 'v'},
#endif
            {"help",        no_argument, 0, 'h'},
            {"version",     no_argument, 0, 'V'},
            {0, 0, 0, 0},
        };
        static string short_options
            (long_options_to_short_options(long_options));
        int option_index;
        int c;

        c = getopt_long(argc, argv, short_options.c_str(),
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'd':
            daemon_flag = true;
            break;
            
        case 'g':
            gui_flag = true;
            break;

        case 'u':
            reliable = false;
            break;

        case 'i':
            interfaces.push_back(optarg);
            break;

        case 'h':
            usage(program_name);
            break;

        case 'v':
#ifndef LOG4CXX_ENABLED
            if (!optarg) {
                verbose = true;
            } else {
                verbosity.push_back(optarg);
            }
#else
            verbose = true;
#endif
            break;

        case 'c':
            conf_dirs.clear();
            conf_dirs.push_back(optarg);
            break;

        case 'l':
            lib_dirs.push_front(optarg);
            break;

        case 'p':
            pid_file = optarg;
            break;

        case 'n':
            info_file = optarg;
            break;

        case OPT_CHECK_LEAKS:
            leak_checker_start(optarg);
            break;

        case OPT_LEAK_LIMIT:
            leak_checker_set_limit(strtoll(optarg,NULL,10));
            break;

        case 'V':
            hello(program_name);
            exit(EXIT_SUCCESS);
            
        case '?':
            exit(EXIT_FAILURE);

        default:
            abort();
        }
    }

    /* Spawn GUI if configured */
    if (gui_flag && start_gui()) {
        exit(EXIT_FAILURE);
    }

    if (!verbose && !daemon_flag) {
        hello(program_name);
    }
    
    /* Determine the end-user applications to run */
    Application_list applications = parse_application_list(optind, argc, argv);
    json_object* platform_configuration;
        
    try {
        /* Parse the platform configuration file */
        
        BOOST_FOREACH(string s, conf_dirs) {
            boost::filesystem::path confpath(s);
            if (!boost::filesystem::exists(confpath)) { continue; }
            
            platform_configuration = json::load_document(s);
            
            /* Try only the first found JSON configuration file, ... */
            if (platform_configuration->type == json_object::JSONT_NULL) {
                throw runtime_error
                    ("Unable to parse the platform configuration file '" + s);
            }

            break;
        }

        /* ... and give up, if it didn't load/pass the json validation. */
        if (!platform_configuration) {
            string err = "Unable to find a configuration file. "
                "Checked the following locations:\n";
            BOOST_FOREACH(string l, conf_dirs) { err += "\t -> " + l + "\n"; }
            throw runtime_error(err);
        }
    }
    catch (const runtime_error& e) {
        printf("ERR: %s", e.what());
        exit(EXIT_FAILURE);
    }
    
    /* Become a daemon before finishing the booting, if configured */
    if (daemon_flag && daemon()) {
        exit(EXIT_FAILURE);
    }

    /* Dump stack traces only in a debug or profile build, since only
       then the frame pointers necessary for stack traces are
       available. */
#ifndef NDEBUG
    register_fault_handlers();
#else
#ifdef PROFILING
    register_fault_handlers();
#endif
#endif

    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);

    init_log();

#ifndef LOG4CXX_ENABLED
    BOOST_FOREACH (const string& s, verbosity) {
        set_verbosity(s.c_str());
    }
#endif

    lg.info("Starting %s (%s)", program_name, argv[0]);
            
    try {
        /* write PID file first */ 
        if (daemon_flag) {
            FILE *f = ::fopen(pid_file, "w");
            if (f) {
                fprintf(f, "%ld\n", (long)getpid());
                fclose(f);
                lg.info("wrote pidfile: %s \n", pid_file); 
            } else {
                throw runtime_error("Couldn't create pid file \"" + 
                                    string(pid_file) + "\": " + 
                                    string(strerror(errno)));
            }
        }

        /* Boot the container */
        nox::init();
        Kernel::init(info_file, argc, argv);
        Kernel* kernel = Kernel::get_instance();
    
        /* Pass the command line arguments for the kernel before
           anything is deployed, so that even the hardcoded components
           could have arguments. */
        BOOST_FOREACH(Application_params app, applications) {
            kernel->set_arguments(app.first, app.second);
        }

        /* Install the built-in statically linked core components:
           event dispatcher and the DSO deployer. */
        kernel->install
            (new Static_component_context
             (kernel, "built-in event dispatcher", 
              boost::bind(&EventDispatcherComponent::instantiate, _1, _2),
              typeid(EventDispatcherComponent).name(),
              platform_configuration), INSTALLED);

        /* Install the deployer responsible for DSOs. */
        kernel->install
            (new Static_component_context
             (kernel, "built-in DSO deployer",
              boost::bind(&DSO_deployer::instantiate, kernel, lib_dirs, _1, _2),
              typeid(DSO_deployer).name(),              
              platform_configuration), INSTALLED);
              
#ifdef TWISTED_ENABLED
        /* Boot the Python deployer/runtime component, responsible for
           all Python components. */
        kernel->install("python", INSTALLED);
        
#endif

        /* Finish the booting on its own thread, so that pollables are
           serviced properly by nox::run() during the installation. */
        Co_thread install(boost::bind(&finish_booting, kernel, applications, 
                                      interfaces, reliable));

        if (daemon_flag) {
            /* PID file will be removed just before the daemon
               exits. Register to the shutdown event to catch SIGTERM,
               SIGINT, and SIGHUP based exits. */
            nox::register_handler(Shutdown_event::static_get_name(), 
                                  boost::bind(&remove_pid_file, pid_file, _1), 
                                  9998);
        }

        nox::run();
    }
    catch (const runtime_error& e) {
        lg.err("%s", e.what());
        exit(EXIT_FAILURE);
    }

    return 0;
}

static void finish_booting(Kernel* kernel, const Application_list& applications,
                           vector<string> interfaces, bool reliable) {
    string fatal_error;

    try {
        /* Boot the components defined on the command-line */
        BOOST_FOREACH(Application_params app, applications) {
            Application_name& name = app.first;
            if (!kernel->get(name)) {
                kernel->install(name, INSTALLED);
            }
        }
    } 
    catch (const runtime_error& e) {
        fatal_error = e.what();
    }

    /* Report the installation results. */
    lg.dbg("Application installation report:");
    BOOST_FOREACH(Component_context* ctxt, kernel->get_all()) {
        lg.dbg("%s:\n%s\n", ctxt->get_name().c_str(),
               ctxt->get_status().c_str());
    }

    /* Check all requested applications have booted. */
    if (fatal_error != "") {
        lg.err("%s", fatal_error.c_str());
        exit(EXIT_FAILURE);
    }

    try {
        /* Connect the openflow interfaces */
        BOOST_FOREACH (string& interface, interfaces) {
            nox::connect(Openflow_connection_factory::create(interface),
                         reliable);
        }
    }
    catch (const runtime_error& e) {
        lg.err("%s", e.what());
        exit(EXIT_FAILURE);
    }

    nox::post_event(new Bootstrap_complete_event());
    lg.info("nox bootstrap complete");
}

