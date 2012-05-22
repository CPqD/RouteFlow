/* Copyright 2008 (C) Nicira, Inc.
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
#include <config.h>
#include "fault.hh"
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>

#include <boost/format.hpp>

#ifdef TWISTED_ENABLED
#include <Python.h>
#include "frameobject.h"

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 4
typedef ssize_t Py_ssize_t;
#endif // Python version <= 2.4
#endif // TWISTED_ENABLED

#define ARRAY_SIZE(ARRAY) (sizeof ARRAY / sizeof *ARRAY)

namespace vigil {

/* Did we register the fault handlers? */
static bool registered;

/* Thread-specific key for keeping track of the signal stack. */
static pthread_key_t signal_stack_key;

#ifdef TWISTED_ENABLED
/* Readable memory regions. */
struct Region {
    uintptr_t start;            /* First byte of region. */
    uintptr_t end;              /* One past last byte of region. */
};
static Region regions[512];
static int n_regions;

/* Reads /proc/self/maps to determine what regions of memory we are allowed to
 * access, to avoid segmentation faults when we're backtracing ourselves. */
static void
read_mem_map()
{
    static bool read_regions = false;
    if (read_regions) {
        return;
    }
    read_regions = true;

    FILE *f = fopen("/proc/self/maps", "r");
    if (f == NULL) {
        fprintf(stderr, "/proc/self/maps: %s\n", strerror(errno));
        return;
    }

    static char line[1024];
    for (int ln = 1;
         fgets(line, sizeof line, f) && n_regions < ARRAY_SIZE(regions);
         ln++) {
        Region& r = regions[n_regions];
        char readable;
        if (sscanf(line, "%"SCNxPTR"-%"SCNxPTR" %c",
                   &r.start, &r.end, &readable) != 3) {
            fprintf(stderr, "/proc/self/maps:%d: parse error\n", ln);
            continue;
        }
        if (readable == 'r') {
            if (n_regions > 0 && r.start == regions[n_regions - 1].end) {
                /* Merge with previous region. */
                regions[n_regions - 1].end = r.end;
            } else {
                /* New region. */
                n_regions++;
            }
        }
    }
    fclose(f);
}

/* Returns true if the 'length' bytes starting at 'p' are readable,
   false otherwise. */
static bool
is_readable(const void* p, uintptr_t length)
{
    uintptr_t addr = (uintptr_t) p;
    for (int i = 0; i < n_regions; i++) {
        Region& r = regions[i];
        if (r.start <= addr && addr <= r.end && addr + length <= r.end) {
            return true;
        }
    }
    return false;
}

/* Dumps 'dict' to stderr, one line per entry. */
static std::string
dump_pydict(PyDictObject* dict)
{
    std::string trace;

    for (Py_ssize_t i = 0; i <= dict->ma_mask; i++) {
        PyDictEntry* entry = &dict->ma_table[i];
        if (!entry->me_key) {
            continue;
        }

        trace += "      ";
        PyObject* v = PyObject_Repr(entry->me_key);
        trace += v ? std::string(PyString_AsString(v)) : "--";
        Py_XDECREF(v);
        trace += " : ";
        v = PyObject_Repr(entry->me_value);
        trace += v ? std::string(PyString_AsString(v)) : "--";
        Py_XDECREF(v);
    }

    return trace;
}

/* Checks whether 'address' is a pointer to a PyFrameObject and, if so, dumps
 * out the filename, line number, and function name of the location it
 * represents, plus the frame's local variables. */
static std::string
dump_python_frame(uintptr_t address)
{
    /* Dereference the pointer and check that it points to a PyFrameObject. */
    PyFrameObject** fop = (PyFrameObject**) address;
    PyFrameObject* fo = *fop;
    if (!is_readable(fo, sizeof *fo) || fo->ob_type != &PyFrame_Type) {
        return "";
    }

    /* Print the filename, line number, and function name */
    PyCodeObject* co = fo->f_code;
    PyStringObject* file = (PyStringObject*) co->co_filename;
    PyStringObject* function = (PyStringObject*) co->co_name;
    std::string trace = 
        boost::str(boost::format("    \%s:%d: %s\n") 
                   % file->ob_sval % fo->f_lineno % function->ob_sval);

    /* Print local variables. */
    PyFrame_FastToLocals(fo);
    PyObject* locals = fo->f_locals;
    if (is_readable(locals, sizeof *locals) && PyDict_CheckExact(locals)) {
        trace += dump_pydict((PyDictObject*) locals);
    }

    return trace;
}
#endif // TWISTED_ENABLED

/* Return a backtrace. */
std::string
dump_backtrace()
{
    std::string trace;

#ifdef TWISTED_ENABLED
    read_mem_map();
#endif
    /* During the loop:

       frame[0] points to the next frame.
       frame[1] points to the return address. */
    for (void** frame = (void**) __builtin_frame_address(0);
         frame != NULL && 
#ifdef TWISTED_ENABLED
             is_readable(frame, sizeof(void*)) &&
#endif
             frame[0] != NULL && frame != frame[0];
         frame = (void**) frame[0]) {
        /* Translate return address to symbol name. */
        Dl_info addrinfo;
        if (!dladdr(frame[1], &addrinfo) || !addrinfo.dli_sname) {
            trace += boost::str(boost::format("  0x%08"PRIxPTR"\n")
                                % (uintptr_t) frame[1]);
            continue;
        }

        /* Demangle symbol name. */
        const char *name = addrinfo.dli_sname;
        char* demangled_name = NULL;
        int err;
        demangled_name = __cxxabiv1::__cxa_demangle(name, 0, 0, &err);
        if (!err) {
            name = demangled_name;
        }

        /* Calculate size of stack frame. */
        void** next_frame = (void**) frame[0];
        size_t frame_size = (char*) next_frame - (char*) frame;

        /* Print. */
        trace += 
            boost::str(boost::format("  0x%08"PRIxPTR" %4u (%s+0x%x)\n")
                       % (uintptr_t) frame[1] % frame_size % name
                       % ((char*) frame[1] - (char*) addrinfo.dli_saddr));

#ifdef TWISTED_ENABLED
        /* If it's a Python frame, extract the Python details. */
        if (!strncmp(name, "PyEval_EvalFrame", 16)) {
            /* Our current frame has PyEval_EvalFrame as the return address.
             * The *next* stack frame is the one with PyEval_EvalFrame's
             * arguments and local variables. */
            void** next_frame = (void**) frame[0];

            /* PyEval_EvalFrame and PyEval_EvalFrameEx take a PyFrameObject* as
             * their first argument, which on x86 makes it the last argument
             * pushed on the stack, just above the frame pointer and return
             * address. */
            trace += dump_python_frame((uintptr_t) &next_frame[2]);
        }
#endif // TWISTED_ENABLED
        free(demangled_name);
    }

    return trace;
}

/* Attempt to demangle a undefined symbol name in an error message.
   The undefined symbol is expected to have a prefix 'undefined
   symbol: ' */
std::string demangle_undefined_symbol(const std::string& cause) {
    using namespace std;

    static const string undef_symbol = "undefined symbol: ";
    string::size_type pos_start = cause.find(undef_symbol);
    
    if (pos_start == string::npos) {
        return cause;
    }

    pos_start += undef_symbol.length();

    string::size_type pos_end = cause.find(" ", pos_start + 1);
    pos_end = 
        pos_end == string::npos ? cause.find("\n", pos_start + 1) : pos_end;
    pos_end = 
        pos_end == string::npos ? cause.find("\t", pos_start + 1) : pos_end;

    const string symbol_name = pos_end == string::npos ? 
        cause.substr(pos_start) :
        cause.substr(pos_start, pos_end - pos_start);
    const string rest = pos_end == string::npos ? "" : cause.substr(pos_end);
        
    size_t demangled_length;
    int status;
    char *demangled =
        __cxxabiv1::__cxa_demangle(symbol_name.c_str(), 0, 
                                   &demangled_length, &status);
    const string demangled_symbol_name = demangled ? demangled : symbol_name;
    free(demangled);
    return cause.substr(0, pos_start) + demangled_symbol_name + rest;
}

void fault_handler(int sig_nr)
{
    fprintf(stderr, "Caught signal %d.\n", sig_nr);
    const std::string trace = dump_backtrace();
    fprintf(stderr, "%s", trace.c_str());
    fflush(stderr);
    signal(sig_nr, SIG_DFL);
    raise(sig_nr);
}

static void
setup_signal(int signo) 
{
    struct sigaction sa;
    sa.sa_handler = fault_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(signo, &sa, NULL)) {
        fprintf(stderr, "sigaction(%d) failed: %s\n", signo, strerror(errno));
    }
}

/* Register a signal stack for the current thread. */
void
create_signal_stack()
{
    if (!registered) {
        /* Don't waste memory on a signal stack if we haven't registered
         * signal handlers anyway.  (Also, we haven't initialized
         * signal_stack_key.) */
        return;
    }

    stack_t oss;
    if (!sigaltstack(NULL, &oss) && !(oss.ss_flags & SS_DISABLE)) {
        /* Already have a signal stack.  Don't create a new one. */
        return;
    }

    stack_t ss;
    /* ss_sp is char* in BSDs and not void* as typically. */
    ss.ss_sp = (char *)malloc(SIGSTKSZ);
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (!ss.ss_sp) {
        fprintf(stderr, "sigaltstack failed: %s\n", strerror(errno));
    } else if (sigaltstack(&ss, NULL)) {
        fprintf(stderr, "Failed to allocate %zu bytes for signal stack: %s\n",
                (size_t) SIGSTKSZ, strerror(errno));
    }

    /* Copy a pointer to the signal stack into a thread-local variable.  Then,
     * if a leak detector such as "valgrind --leak-check=full" is used, the
     * signal stack will show up as a leak if and only if the thread is
     * terminated without calling free_signal_stack(). */
    pthread_setspecific(signal_stack_key, ss.ss_sp);
}

/* Destroy the thread's signal stack. */
void
free_signal_stack()
{
    if (!registered) {
        /* We haven't registered signal handlers, so there's no way that we
         * could have a signal stack to free. */
        return;
    }

    stack_t ss;
    memset(&ss, 0, sizeof ss);
    ss.ss_flags = SS_DISABLE;

    stack_t oss;
    if (!sigaltstack(&ss, &oss) && !(oss.ss_flags & SS_DISABLE)) {
        pthread_setspecific(signal_stack_key, NULL);
        free(oss.ss_sp);
    }
}

void register_fault_handlers()
{
    if (registered) {
        return;
    }
    registered = true;

    if (int error = pthread_key_create(&signal_stack_key, free)) {
        fprintf(stderr, "pthread_key_create");
        fprintf(stderr, " (%s)", strerror(error));
        exit(EXIT_FAILURE);
    }
    setup_signal(SIGABRT);
    setup_signal(SIGBUS);
    setup_signal(SIGFPE);
    setup_signal(SIGILL);
    setup_signal(SIGSEGV);
}

} // namespace vigil
