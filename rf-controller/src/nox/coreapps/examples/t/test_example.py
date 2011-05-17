from nox.lib.core import *
from nox.coreapps.testharness.testdefs import *

import logging

import os

lg = logging.getLogger('test_example')

class test_example(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def configure(self, configuration):
        pass

    def getInterface(self):
        return str(test_example)

    def bootstrap_complete_callback(self, *args):
        nox_test_assert(1, "test_example")
        os._exit(0)  ## CHANGEME!!

    def install(self):
        self.register_for_bootstrap_complete(self.bootstrap_complete_callback)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return test_example(ctxt)

    return Factory()
