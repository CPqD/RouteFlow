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
#include "config.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE NOX
#define BOOST_TEST_MAIN
#define BOOST_TEST_NO_MAIN

#include <boost/bind.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/results_reporter.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <deque>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "assert.hh"
#include "bootstrap-complete.hh"
#include "component.hh"
#include "static-deployer.hh"
#ifdef TWISTED_ENABLED
#include "pyrt/pycomponent.hh"
#include "pyrt/pycontext.hh"
#include "pyrt/pyglue.hh"
#include "swigpyrun.h"
#endif
#include "threads/cooperative.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace boost::unit_test;

#ifdef TWISTED_ENABLED
using namespace vigil::applications;
#endif

static Vlog_module lg("tests");

#ifdef TWISTED_ENABLED
namespace {

Co_sema* sem;

}

namespace vigil {

// note: this errback is only registered if the python test case
// returns a deferred.  it is NOT an errback for the Twisted unit
// testing framework
void vigil_test_errback(PyObject* err) {
    lg.err("Python Twisted unit test framework returned an error, "
           "while it shouldn't. Aborting the tests.");
    PyObject_Print(err, stderr, 0);
    exit(1);
}


// used to allow python tests to be asynchronous.  if a python test
// returns a deferred, then the testing infrastructure will wait until
// for a callback on that deferred before continuing. 
void vigil_test_callback(PyObject*) {
    sem->up();
}

}
#endif

namespace {

#ifdef TWISTED_ENABLED

class PyFixture 
    : public PyComponent { 
public:
    PyFixture(const Context* ctxt, 
              PyObject* pyt) 
        : PyComponent(ctxt, pyt) {
        test_result = 0;
        errback = 0;
        callback = 0;
    }
    
    void testPython() {
        // Construct a twisted.trial.reporter.TestResult object
        PyObject* n = PyString_FromString("twisted/trial/reporter");
        PyObject* m = PyImport_Import(n);
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
            BOOST_FAIL("Unable to import twisted.trial.reporter.");
        }        

        PyObject* f = PyObject_GetAttrString(m, (char*)"TestResult");
        BOOST_REQUIRE_MESSAGE(f && PyCallable_Check(f), 
                              "Unable to find twisted.trial."
                              "reporter.TestResult class");

        test_result = PyObject_CallObject(f, NULL);
        BOOST_REQUIRE_MESSAGE(test_result, 
                              "Unable to construct a twisted.trial."
                              "reporter.TestResult.");
        
        n = PyString_FromString("nox.netapps.tests.pytests");
        m = PyImport_Import(n);
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
            BOOST_FAIL("Unable to import nox.netapps.tests.pytests.");
        }        

        callback = PyObject_GetAttrString(m, (char*)"vigil_test_callback");
        errback  = PyObject_GetAttrString(m, (char*)"vigil_test_errback");
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
            BOOST_FAIL("Unable to initialize the callbacks.");
        }

        sem = new Co_sema();
        // TODO: destroy the semaphore as well.

        // Call the test case and use the type of the return value to
        // determine the type of the test case (blocking
        // vs. non-blocking).
        PyObject* run = PyString_FromString("run");
        PyObject* d = 
            PyObject_CallMethodObjArgs(pyobj, run, test_result, 0);
        Py_XDECREF(run);
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
            
            Py_XDECREF(d);
            BOOST_FAIL("Unable to execute Python TestCase.run()");
            
        } else if (d == Py_None) {
            // Test run complete, there is no deferral to wait on.
            Py_XDECREF(d);
            sem->up();
            
        } else {
            // It's a Twisted deferred. Set the timer, add our combined 
            // callback / errback to determine test run completion.
            PyObject* n = PyString_FromString("addCallbacks");
            PyObject* r = PyObject_CallMethodObjArgs(d, n, callback, 
                                                     errback, 0);
            Py_XDECREF(r);
            Py_XDECREF(n);
            Py_XDECREF(d);

            if (PyErr_Occurred()) {
                PyErr_Print();
                PyErr_Clear();
                BOOST_FAIL("Unable to call Deferred.addCallbacks()");
            }            
        }
        
        // wait for deferal to complete
        sem->down();

        // Inspect the results
        PyObject* failures = PyObject_GetAttrString(test_result, 
                                                    (char*)"failures");
        if (PyList_Size(failures) > 0) {
            PyObject* failure_tuple = PyList_GetItem(failures, 0);
            PyObject* failure_failure = PyTuple_GetItem(failure_tuple, 1);

            Py_XDECREF(d);
            PyObject* f = PyObject_GetAttrString(failure_failure, 
                                                 (char*)"getBriefTraceback");
            PyObject* traceback = PyObject_CallObject(f, 0);
            Py_DECREF(f);

            try {
                BOOST_FAIL(PyString_AsString(traceback));
            } catch (...) {
                Py_DECREF(traceback);
                throw;
            }
        }
        Py_DECREF(failures);

        PyObject* errors = PyObject_GetAttrString(test_result, (char*)"errors");
        if (PyList_Size(errors) > 0) {
            PyObject* error_tuple = PyList_GetItem(errors, 0);
            PyObject* error_failure = PyTuple_GetItem(error_tuple, 1);

            Py_XDECREF(d);
            PyObject* f = PyObject_GetAttrString(error_failure, 
                                                 (char*)"getBriefTraceback");
            PyObject* traceback = PyObject_CallObject(f, 0);
            Py_DECREF(f);

            try {
                BOOST_FAIL(PyString_AsString(traceback));
            } catch (...) {
                Py_DECREF(traceback);
                throw;
            }
        }
        
        Py_DECREF(errors);
        Py_XDECREF(test_result);
        Py_XDECREF(callback);
        Py_XDECREF(errback);
    }
    
private:
    PyObject* test_result;
    PyObject* callback;
    PyObject* errback;
};  

static Component* get_fixture_instance(PyObject* pyt, const Context* c, 
                                       const json_object* xml) {
    return new PyFixture(c, pyt);
}

#endif

class Tests
    : public Component {
public:
    Tests(const Context* c,
          const json_object*) 
        : Component(c), stream(&std::cerr) {
    }

    ~Tests() { 

    }

    void configure(const Configuration* conf) {
        args = conf->get_arguments();

        unit_test_log.set_stream(cout);
        results_reporter::set_stream(cout);

        // possible args = HRF/XML
        parameters.push_back("output_format=HRF");
        // Show status bar
        parameters.push_back("show_progress=yes");
        // possible args = no, confirm, short, detailed
        parameters.push_back("report_level=short");
    }

    void install() {
        /* Run the tests only once the boot is complete */
        register_handler<Bootstrap_complete_event>
            (boost::bind(&Tests::handle_bootstrap, this, _1));
    }

    Disposition handle_bootstrap(const Event&) {
        thread.start(boost::bind(&Tests::run, this, args));
        return CONTINUE;
    }

private:
    Co_thread thread;
    deque<string> parameters;
    ostream* stream;
    list<string> args;

#ifdef TWISTED_ENABLED
    /*
     * Gather and initialize the Python test suites.
     */
    vector<test_case*> gather_python_tests(list<string> args) {
        // Construct the PyContext object and wrap the C++ context object
        // with it.
        PyObject* m = PyImport_ImportModule("nox.coreapps.pyrt.pycomponent");
        if (!m) {
            PyErr_Print();
            PyErr_Clear();
            throw runtime_error("Could not retrieve a Python context module");
        }
        
        PyContext* p = new PyContext(const_cast<Context*>(ctxt), this,
                                     new Python_event_manager());
        swig_type_info* s = SWIG_TypeQuery("_p_vigil__applications__PyContext");
        if (!s) {
            Py_DECREF(m);
            throw runtime_error("Could not find PyContext SWIG type_info.");
        }
        
        PyObject* pyctxt = SWIG_Python_NewPointerObj(p, s, 0);
        assert(pyctxt);

        Py_DECREF(m);

        m = PyImport_ImportModule("nox.netapps.tests.unittest");
        if (!m) {
            PyErr_Print();
            PyErr_Clear();
            throw runtime_error("Unable to import nox.netapps.tests.unittest.");
        }        

        PyObject* g = PyModule_GetDict(m);
        
        PyObject* f = PyDict_GetItemString(g, (char*)"gather_tests");    
        PyObject* t = PyTuple_New(2);
        PyTuple_SetItem(t, 0, to_python_tuple(args));
        PyTuple_SetItem(t, 1, pyctxt);
        
        PyObject* test_suites = PyObject_CallObject(f, t);
        Py_DECREF(t);
        Py_DECREF(m);
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
            throw runtime_error("Unable to gather the Python unit tests.");
        }        
        
        // Convert the Python test suites to Boost.Test test suites.
        vector<test_case*> ts;
        while (PyList_Size(test_suites) > 0) { 
            // Pop up the next test case to run.
            PyObject* test_suite = PyList_GetItem(test_suites, 0);
            PyObject* test_module_name = PyTuple_GetItem(test_suite, 0);
            PyObject* test_module_cases = PyTuple_GetItem(test_suite, 1);
            
            for (int i = 0; i < PyList_Size(test_module_cases); ++i) {
                PyObject* test_case = PyList_GetItem(test_module_cases, i);
                PyObject* test_method_getter = PyString_FromString("getTestMethodName");
                PyObject* test_method_name = 
                    PyObject_CallMethodObjArgs(test_case, test_method_getter, 0);
                if (PyErr_Occurred()) {
                    PyErr_Print();
                    PyErr_Clear();
                    throw runtime_error("Unable to get method name");
                }                   
                
                Py_DECREF(test_method_getter);

                // Wrap the Python test case into PyComponent and
                // deploy it into the container.
                string test_name = 
                    string(PyString_AsString(test_module_name)) + "." +
                    string(PyString_AsString(test_method_name));

                try {
                    ctxt->get_kernel()->install
                        (new Static_component_context
                         (ctxt->get_kernel(), test_name,
                          boost::bind(&get_fixture_instance, test_case, _1, _2),
                          typeid(PyComponent).name(), 0), INSTALLED);
                    PyFixture* pf = 
                        dynamic_cast<PyFixture*>(ctxt->get_by_name(test_name));
                    assert(pf);
                    ts.push_back(new boost::unit_test::
                                 test_case(test_name, 
                                           boost::bind(&PyFixture::testPython,
                                                       pf)));
                }
                catch (const runtime_error& e) {
                    Py_DECREF(test_method_name);
                    throw runtime_error("Unable to install Python test case: " +
                                        string(e.what()));
                }
                
                Py_DECREF(test_method_name);
            }

            PyObject* new_test_suites = 
                PyList_GetSlice(test_suites, 1, PyList_Size(test_suites));
            Py_DECREF(test_suites);
            test_suites = new_test_suites;
        }

        Py_DECREF(test_suites);
        
        return ts;
    }
#endif
    
    void run(list<string> args) {
#ifdef TWISTED_ENABLED
        vector<test_case*> test_cases = gather_python_tests(args);

        // Auto-register all Python test cases.
        BOOST_FOREACH(test_case* tc, test_cases) {
            ut_detail::auto_test_unit_registrar r(tc, 0);
        }
#endif

        parameters.push_front("catch_system_errors=no");
        
        // unit_test_main() expects argv[0] to be a program name, so
        // invent there something.
        int argc = 0;
        char *argv[parameters.size() + 1];
        argv[argc++] = const_cast<char*>("prog");
        
        for (deque<string>::const_iterator i = parameters.begin(); 
             i != parameters.end(); ++i) {
            argv[argc++] = ::strdup(const_cast<char*>(("--" + *i).c_str()));
        }
        
        int result = unit_test_main(&init_unit_test, argc, argv);
        stream->flush();
        exit(result);
    }
};

REGISTER_COMPONENT(container::Simple_component_factory<Tests>, Tests);

} // unnamed namespace

