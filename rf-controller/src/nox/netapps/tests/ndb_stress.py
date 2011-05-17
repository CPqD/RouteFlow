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
from twisted.python import log

from nox.apps.ndb import API, GetOp, PutOp
from nox.apps.tests import unittest

pyunit = __import__('unittest')

class NDBStressCase(unittest.TestCase):
    """
    Testing NDB by adding and deleting 10,000 elements.  Verifies test
    by ensuring final select query for * returns 0 items
    """

    def configure(self, config):
        self.ndb = self.resolve(API)

    def getInterface(self):
        return str(NDBStressCase)    

    def setUp(self):
        """
        Deploy the schema.
        """
        return self.ndb.drop_table('TEST').\
            addCallback(lambda x : self.ndb.create_table('TEST', {'NAME': "", 
                                                                  'IP': 1}, 
                                                         []))
 
    def tearDown(self):
        pass

    def verifyDel(self,results):    
        query = {}

        def _verify(results):
            self.failUnlessEqual(len(results), 1, 'Too many results')
            results = results[0]
            self.failUnlessEqual(len(results.results), 0, 'Not empty?')

        op = GetOp('TEST', query)
        d = self.ndb.execute([ op ])
        d.addCallback(_verify)
        return d

    def delPuts(self, results):
        """
        Delete all put entries 
        """
        ops = []
        for i in range(0,10000):
            ops.append(PutOp('TEST', None, {'NAME' : str(i)}))

        d = self.ndb.execute(ops)
        d.addCallback(self.verifyDel)
        return d

    def testPutDel(self):
        """
        Write 10,000 entries 
        """
        ops = []
        for i in range(0,10000):
            ops.append(PutOp('TEST', {'NAME' : str(i), 'IP' : i}))

        d = self.ndb.execute(ops)
        d.addCallback(self.delPuts)
        return d

def suite(ctxt):
    suite = pyunit.TestSuite()
    print "warning: commented out ndb_stress.py test to speed up test suite"
    print "re-enable the test once we can run tests individually"
#   DW: commenting this out so the rest of the tests run at a reasonable speed
#    suite.addTest(NDBStressCase("testPutDel", ctxt))
    return suite
