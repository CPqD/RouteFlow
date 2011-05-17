def nox_test_verbose(v):
    if v:
        print "+verbose: True"
    else:
        print "+verbose: False"

def nox_test_debug(v):
    if v:
        print "+debug: True"
    else:
        print "+debug: False"

def nox_test_init(runtime=5):
    print "+runtime:", str(runtime)

def nox_test_msg(msg):
    print "+print_msg:", msg

def nox_test_output(msg):
    print "+output:", msg

def nox_test_fail(name=""):
    print "+assert_fail:", name

def nox_test_pass(name=""):
    print "+assert_pass:", name

def nox_test_assert(expr, name=""):
    if expr:
        nox_test_pass(name)
    else:
        nox_test_fail(name)

def nox_test_fail_unless(expr, name=""):
    nox_test_assert(expr, name)

#import io

class NoxTestOutputStream:
    def writeable(self):
        return True

    def writelines(self, lines):
        for l in lines:
            nox_test_output(l.rstrip())

    def write(self, msg):
        self.writelines(msg.splitlines())

import unittest
from unittest import TestCase as NoxTestCase

class _NoxTestResult(unittest.TestResult):
    def __init__(self):
        self.ostream = NoxTestOutputStream()
        unittest.TestResult.__init__(self)
        self.shouldStop = False

    def getDescription(self, test):
        return test.shortDescription() or str(test)

    def startTest(self, test):
        unittest.TestResult.startTest(self, test)

    def stopTest(self, test):
        unittest.TestResult.stopTest(self, test)

    def addSuccess(self, test):
        unittest.TestResult.addSuccess(self, test)
        nox_test_pass(self.getDescription(test))

    def addError(self, test, err):
        unittest.TestResult.addError(self, test, err)
        nox_test_fail(self.getDescription(test))
        self.ostream.write(self.errors[-1][1])

    def addFailure(self, test, err):
        unittest.TestResult.addFailure(self, test, err)
        nox_test_fail(self.getDescription(test))
        self.ostream.write(self.failures[-1][1])

    def printErrors(self):
        pass

class NoxTestRunner:
    def __init__(self, runtime=30, verbose=None, debug=None):
        nox_test_init(runtime)
        if debug != None:
            nox_test_debug(debug)
        if verbose != None:
            nox_test_verbose(verbose)

    def run(self, test):
        result = _NoxTestResult()
        test(result)
        return result

def nox_test_main(runtime=30, verbose=None, debug=None):
    runner=NoxTestRunner(runtime=runtime, verbose=verbose, debug=debug)
    unittest.main(testRunner=runner)
