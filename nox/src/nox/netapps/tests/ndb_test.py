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
import random
from twisted.internet.defer import Deferred
from twisted.internet import reactor
from twisted.python import log
from nox.apps.ndb import API, GetOp, PutOp
from nox.apps.tests import unittest

pyunit = __import__('unittest')

class NDBTestCase(unittest.TestCase):
    """
    Hello world test executes a series of operations:

    1) drop and create a table (with random table name)
    2) put(),
    3) get(),
    4) put() replacing rows fetched earlier (i.e., corresponds to modify()), and
    5) check 4 did trigger invalidation callbacks.
    
    The test is agnostic to the database connectivity: it remains
    unchanged regardless whether it's directly to the master database
    or through the cache. (For future purposes.)
    """

    specificQuery1Invalidated = False
    specificQuery2Invalidated = False
    allQueryInvalidated = False

    def configure(self, config):
        self.ndb = self.resolve(API)

    def getInterface(self):
        return str(NDBTestCase)    

    def setUp(self):
        """
        Create the table.
        """    
        self.table = ''       
        for c in xrange(0,10):
            self.table += chr(random.randint(65, 90))
        return self.ndb.drop_table(self.table).\
            addCallback(lambda x : self.ndb.create_table(self.table, {"T": "", 
                                                                      "I": 1,
                                                                      "F": 0.0,
                                                                      "B": buffer("foo")}, 
                                                         []))
    
    def testPut(self):
        """
        Write two entries.
        """
        log.msg("******************** put_hello ********************")
        self.row1 = {'T' : 'FOO', 
                     'B' : buffer("foobar"),
                     'F' : 1.023,
                     'I' : None}
        self.row2 = {'T' : 'BAR', 'I' : 3232235791}
        
        self.op1 = PutOp(self.table, self.row1)
        self.op2 = PutOp(self.table, self.row2)
        d = self.ndb.execute([ self.op1, self.op2 ])
        d.addCallback(self.testGet)
        return d

    def testGet(self, ignore):
        """
        Read the previously written rows, with exact match.
        """
        log.msg("******************** get_hello ********************")
        query1 = self.row1
        query2 = self.row2
        
        def results_invalidated_1():
            log.msg(">>>>>>>>>>>>>>>>>>>> 1 Previous query results (criteria = %s) for %s have been invalidated." % (str(query1), self.table))
            self.specificQuery1Invalidated = True

        def results_invalidated_2():
            log.msg(">>>>>>>>>>>>>>>>>>>> 2 Previous query results (criteria = %s) for %s have been invalidated." % (str(query2), self.table))
            self.specificQuery2Invalidated = True

        self.op1 = GetOp(self.table, query1, results_invalidated_1)
        self.op2 = GetOp(self.table, query2, results_invalidated_2)
        d = self.ndb.execute([ self.op1, self.op2 ])        
        d.addCallback(self.validateGetResults)
        return d
    
    def validateGetResults(self, results):
        log.msg("execute results are: %s" % 
                map(lambda x: str(x), results))
        log.msg('query1 results = %s' % str(self.op1.results))
        log.msg('query2 results = %s' % str(self.op2.results))
        self.failUnlessEqual(len(self.op1.results), 1)
        self.failUnlessEqual(len(self.op2.results), 1)
        self.failUnlessEqual(self.op1.results, results[0].results)
        self.failUnlessEqual(self.op2.results, results[1].results)
        
        return self.testInvalidation()

    def testInvalidation(self):
        """
        Read all previously written rows with a single query.
        """
        log.msg("******************** invalidation_hello ********************")
        
        query = {}

        def results_invalidated():
            log.msg(">>>>>>>>>>>>>>>>>>>> Previous query results (criteria = all) for %s have been invalidated." % self.table)
            self.allQueryInvalidated = True

        self.op = GetOp(self.table, query, results_invalidated)
        d = self.ndb.execute([ self.op ])        
        d.addCallback(self.validateGetAllResults)
        return d
    
    def validateGetAllResults(self, results):
        log.msg("test_invalidation_get results are: %s" % 
                map(lambda x: str(x), results))
        self.failUnlessEqual(self.op.results, results[0].results)
        
        return self.testModify(results)

    def testModify(self, results):
        """
        Modify a written row, but only if the previously fetched rows have
        not changed since.
        """

        log.msg("******************** modify_hello ********************")

        op = PutOp(self.table, {'T' : 'BAR', 'I' : None}, {'T': 'BAR'})
        d = self.ndb.execute([ op ], results)
        d.addCallback(self.validateModify)
        return d
    
    def validateModify(self, results):
        """ 
        Check the row was modified properly.
        """
        query = {'T': 'BAR'}
        self.op = GetOp(self.table, query)
        d = self.ndb.execute([self.op])
        d.addCallback(self.validateModify2)
        return d
    
    def validateModify2(self, results):
        
        return self.testDelete()

    def testDelete(self):
        """
        Wipe the previously written rows by replacing them with nothing.
        """
        log.msg("******************** delete_hello ********************")

        op1 = PutOp(self.table, None, {})
        d = self.ndb.execute([ op1 ])
        d.addCallback(self.finalize)
        return d

    def finalize(self, results):
        self.failUnless(self.allQueryInvalidated, "All invalidation failure.")
        self.failUnless(not self.specificQuery1Invalidated, "Specific invalidation failure 1.")
        self.failUnless(self.specificQuery2Invalidated, "Specific invalidation failure 2.")

    def tearDown(self):
        return self.ndb.drop_table(self.table)

def suite(ctxt):
    suite = pyunit.TestSuite()
    #suite.addTest(NDBTestCase("testPut", ctxt, name))
    return suite
