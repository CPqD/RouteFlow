# Copyright 2008 (C) Nicira, Inc.
# 
# This file is part of NOX.
# 
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.
import os

from twisted.trial import itrial, reporter, util, unittest
from twisted.trial.unittest import TestCase, PyUnitResultAdapter
from twisted.internet import defer
from nox.lib.core import Component

pyunit = __import__('unittest')

# location unit tests 
testdir       = 'nox/netapps/tests/pyunittests/'
testnamespace = testdir.replace('/','.')


# subclass of the standard twisted TestCase so that we 
# have an object that is both a TestCase and a Component
class NoxTestCase(unittest.TestCase, Component):
    """
    Test case is a component.
    """
    def __init__(self, methodName, ctxt):
        unittest.TestCase.__init__(self, methodName)
        Component.__init__(self, ctxt)

    def configure(self, configuration):
        pass

    def install(self):
        pass

    def getTestMethodName(self):
        x = self.id()
        return x[x.rfind('.') + 1:]
        
    # this method is mostly copy and past from the main twisted TestCase.run()
    # method of our parent class.  The main difference is that we return a
    # deferred object instead of waiting on it.  As a result, we need to perform
    # our clean-up in a separate function.
    def run(self, result):
        self.res = result #dw: hack so that we can explicitly add errors

        from twisted.internet import reactor
        new_result = itrial.IReporter(result, None)
        if new_result is None:
            result = PyUnitResultAdapter(result)
        else:
            result = new_result
        self._timedOut = False
        if self._shared and self not in self.__class__._instances:
            self.__class__._instances.add(self)
        result.startTest(self)
        if self.getSkip(): # don't run test methods that are marked as .skip
            result.addSkip(self, self.getSkip())
            result.stopTest(self)
            return
        self._installObserver()
        self._passed = False
        first = False
        if self._shared:
            first = self._isFirst()
            self.__class__._instancesRun.add(self)
        self._deprecateReactor(reactor)
        try:
            if first:
                d = self.deferSetUpClass(result)
            else:
                d = self.deferSetUp(None, result)
            cb = lambda x: self.clean_up(result,x)
            d.addBoth(cb) 

        except:
          print "Error launching test: " + self.name
          self.clean_up(result,"error launching test")
          return None
       
        d.addErrback(self.last_errback)

        return d

    def clean_up(self,result, x):
      from twisted.internet import reactor
      try: 
          # NOTE: we comment this out b/c it calls reactor methods
          # that are not implemented by our 'pyoxidereactor'
          # it should be ok, b/c these are just unit tests
          #self._cleanUp(result)
          result.stopTest(self)
          #if self._shared and self._isLast():
          #    self._initInstances()
          #    self._classCleanUp(result)
          #if not self._shared:
          #    self._classCleanUp(result)
      finally: 
        self._undeprecateReactor(reactor)

    # have one last method to catch any errors in clean_up itself
    # keep this one simple, it can't throw an exception itself :) 
    def last_errback(self,x):
      print "Last Errback:"
      print str(x)

def identify_tests():
    tests = []
    for file in os.listdir(testdir):
        fsplit = file.split('.')
        if len(fsplit) != 2:
            continue
        if fsplit[1] != 'py' and fsplit[1] != 'pyc':
            continue
        module = fsplit[0]
        if module == '__init__':
            continue
        tests.append(testnamespace + module)
    return tests

def gather_tests(tests, ctxt):
    suites = []
    if not len(tests) or filter(lambda x: x == '_tests', tests):
        tests = identify_tests()
    tests = filter(lambda x: x.find(testnamespace) == 0, tests)            
    tests = map(lambda x: x.replace(testnamespace,''), tests)            
    hierarchy = {}    
    for test in tests:
        mod_case = test.split('.')
        if not hierarchy.has_key(mod_case[0]):
            hierarchy[mod_case[0]] = {}
        if len(mod_case) > 1:
            hierarchy[mod_case[0]][mod_case[1]] = ''
    for module in hierarchy.keys():
        try:
            name = testnamespace + module
            mod = __import__(name, globals(), locals(), ['suite'], -1)
            suite = pyunit.TestSuite()
            if not hasattr(mod, "suite"):
                continue
            s = getattr(mod, "suite")
            all_suite_tests = s(ctxt)
            if len(hierarchy[module]) > 0:
                matching_tests = []  
                for case in all_suite_tests:
                    if hierarchy[module].has_key(case.getTestMethodName()):  
                        matching_tests.append(case)
                suite.addTests(matching_tests)
            else:  
                suite.addTests(all_suite_tests)

            suites.append((name, suite._tests))
        except TypeError, e:
            raise
        except ImportError, e:
            raise
    return suites
