# Copyright 2008, 2009 (C) Nicira, Inc.
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
import logging

from nox.coreapps.testharness.testdefs import *
from nox.lib.core import Component, CONTINUE
from nox.netapps.storage.t.storage_test_base import StorageTestBase

log = logging.getLogger('storage_test')

class StorageTestCase(StorageTestBase):

    def getInterface(self):
        return str(StorageTestCase)

    def install(self):
        self.register_for_bootstrap_complete(self.bootstrap_complete_callback)

    def bootstrap_complete_callback(self, *args):
        self.setUp()

        self.testAddthenDrop().\
            addCallback(lambda ignore: self.testAddthenPut()).\
            addCallback(lambda ignore: self.testAddthenPutWrongTable()).\
            addCallback(lambda ignore: self.testAddthenPutWrongTable()).\
            addCallback(lambda ignore: self.testAddthenPutthenGet()).\
            addCallback(lambda ignore: self.testAddthenPutthenGetNextUsingIndex()).\
            addCallback(lambda ignore: self.testAddthenPutthenGetNextAll()).\
            addCallback(lambda ignore: self.testAddthenPutthenGetDoubleIndexCheck()).\
            addCallback(lambda ignore: self.testMultiIndex()).\
            addCallback(lambda ignore: self.testMultiIndexModify()).\
            addCallback(lambda ignore: self.testRemoveMultipleRows()).\
            addCallback(lambda ignore: self.testGetRemoveSequence()).\
            addCallback(lambda ignore: self.testCreateSchemaCheck()).\
            addCallback(self.complete).\
            addErrback(self.error)    
        return CONTINUE

    def complete(self, ignore=None):
        os._exit(0)  ## CHANGEME!!

    def error(self, failure=None):
        print failure
        os._exit(1)  ## CHANGEME!!

    def setUp(self):
        try:
            from nox.netapps.storage import Storage
            self.impl = self.ctxt.resolve(str(Storage))
        except Exception, e:
            print e
            os._exit(2)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return StorageTestCase(ctxt)

    return Factory()
