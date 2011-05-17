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

from nox.netapps.tests import unittest
from nox.netapps.storage import Storage, StorageException

pyunit = __import__('unittest')

class TransactionalStorageTestCase(unittest.NoxTestCase):

    def getInterface(self):
        return str(TransactionalStorageTestCase)

    def setUp(self):
        try:
            from nox.ext.apps.tstorage import TransactionalStorage
            self.impl = self.ctxt.resolve(str(TransactionalStorage))
        except Exception, e:
            print e

        # errors
        def dropped_ok(res):
            pass

        def dropped_err(res):
            pass

        def connection_ok(r):
            result, self.conn = r
            self.failUnless(hasattr(self.conn, "create_table"))
            self.failUnless(hasattr(self.conn, "drop_table"))
            self.failUnless(hasattr(self.conn, "get"))
            self.failUnless(hasattr(self.conn, "put"))
            self.failUnless(hasattr(self.conn, "modify"))
            self.failUnless(hasattr(self.conn, "remove"))
            self.failUnless(hasattr(self.conn, "commit"))
            self.failUnless(hasattr(self.conn, "begin"))
            self.failUnless(hasattr(self.conn, "put_table_trigger"))
            self.failUnless(hasattr(self.conn, "put_row_trigger"))
            self.failUnless(hasattr(self.conn, "remove"))
            self.failUnless(hasattr(self.conn, "get_transaction_mode"))

        def connection_err(r):
            self.fail("can't get a connection to storage")

        d = self.impl.get_connection()
        d.addCallbacks(connection_ok, connection_err)
        return d

    def tearDown(self):
        pass

    def testCreateGet(self):
        def get_close(result):
            self.failUnless(result[0] == Storage.SUCCESS)
            #print 'get_next done'

        def get_next(res):
            result, row = res
            self.failUnless(result[0] == Storage.NO_MORE_ROWS)
            d = self.cursor.close()
            d.addCallback(get_close)
            return d

        def get_ok(res):
            result, self.cursor = res
            self.failUnless(result[0] == Storage.SUCCESS)
            #print 'get ok'

            d = self.cursor.get_next()
            d.addCallback(get_next)
            return d

        def get_err(res):
            self.fail("can't open a cursor to storage")

        def get(result):
            self.failUnless(result[0] == Storage.SUCCESS)

            #print 'getting'
            d = self.conn.get('pyunit_transactional_test_table', { })
            d.addCallback(get_ok)
            return d

        def create(result):
            #print 'creating a table2'
            d = self.conn.create_table("pyunit_transactional_test_table",
                                       {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float, "COLUMN_GUID" : tuple}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),), 0)
            d.addCallback(get)
            return d

        d = self.conn.drop_table('pyunit_transactional_test_table')
        d.addCallback(create)
        d.addErrback(create)
        return d

    def testPutGetModifyGetRemove(self):

        def get3_close(result):
            #print 'get3 close'
            self.failUnless(result[0] == Storage.SUCCESS)

        def get3_next(res):
            #print 'get3 next'
            result, self.row = res
            self.failUnless(result[0] == Storage.NO_MORE_ROWS)

            d = self.cursor.close()
            d.addCallback(get3_close)
            return d

        def get3_ok(res):
            #print 'get3 ok'
            result, self.cursor = res
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.cursor.get_next()
            d.addCallback(get3_next)
            return d

        def remove_ok(result):
            #print 'remove ok'
            self.failUnless(result[0] == Storage.SUCCESS)
            self.failUnless(self.row_trigger_called == 3)
            self.failUnless(self.table_trigger_called == 3)

            d = self.conn.get('pyunit_transactional_test_table', { 'GUID' : self.guid })
            d.addCallback(get3_ok)
            return d

        def trigger_ok3(res):
            d = self.conn.remove('pyunit_transactional_test_table', self.row)
            d.addCallback(remove_ok)
            return d

        def trigger_func2(tid, row, reason):
            self.failUnless(self.row['COLUMN_INT'] == 1235)
            self.failUnless(self.row['COLUMN_STR'] == 'string123')
            self.failUnless(self.row['COLUMN_FLOAT'] == 3456.0)
            self.failUnless(reason == Storage.REMOVE)
            self.row_trigger_called += 1

        def get2_close(result):
            #print 'get2 close'
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.conn.put_row_trigger('pyunit_transactional_test_table', {'COLUMN_INT' : 1235, 'COLUMN_STR' : 'string123' }, trigger_func2)
            d.addCallback(trigger_ok3)
            return d

        def get2_next(res):
            #print 'get2 next'
            result, self.row = res
            #print result

            self.failUnless(result[0] == Storage.SUCCESS)

            self.failUnless(self.row['COLUMN_INT'] == 1235)
            self.failUnless(self.row['COLUMN_STR'] == 'string123')
            self.failUnless(self.row['COLUMN_FLOAT'] == 3456.0)
            self.failUnless(self.row['GUID'] == self.guid)

            d = self.cursor.close()
            d.addCallback(get2_close)
            return d

        def get2_ok(res):
            #print 'get ok'
            result, self.cursor = res
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.cursor.get_next()
            d.addCallback(get2_next)
            return d

        def modify_ok(result):
            #print 'modify ok'
            self.failUnless(result[0] == Storage.SUCCESS)
            self.failUnless(self.row_trigger_called == 2)

            d = self.conn.get('pyunit_transactional_test_table', { 'GUID' : self.guid })
            d.addCallback(get2_ok)
            return d

        def trigger_ok2(result):
            row = self.row.copy()
            #print 'foo'
            row['COLUMN_INT'] = 1235
            #print row
            #print self.row

            d = self.conn.modify('pyunit_transactional_test_table', row)
            d.addCallback(modify_ok)
            return d

        def trigger_func(tid, row, reason):
            self.failUnless(self.row['COLUMN_INT'] == 1234)
            self.failUnless(self.row['COLUMN_STR'] == 'string123')
            self.failUnless(self.row['COLUMN_FLOAT'] == 3456.0)
            self.row_trigger_called += 1

        def get_close(result):
            #print 'get close'
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.conn.put_row_trigger('pyunit_transactional_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123' }, trigger_func)
            d.addCallback(trigger_ok2)
            return d

        def get_next(res):
            #print 'get next'
            result, self.row = res
            #print result
            
            self.failUnless(result[0] == Storage.SUCCESS)

            self.failUnless(self.row['COLUMN_INT'] == 1234)
            self.failUnless(self.row['COLUMN_STR'] == 'string123')
            self.failUnless(self.row['COLUMN_FLOAT'] == 3456.0)
            self.failUnless(self.row['GUID'] == self.guid)

            d = self.cursor.close()
            d.addCallback(get_close)
            return d

        def get_ok(res):
            #print 'get ok'
            result, self.cursor = res
            #print self.row_trigger_called
            self.failUnless(self.row_trigger_called == 1)
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.cursor.get_next()
            d.addCallback(get_next)
            return d

        def put_ok(res):
            #print 'put ok'
            result, self.guid = res
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.conn.get('pyunit_transactional_test_table', { 'GUID' : self.guid })
            d.addCallback(get_ok)
            return d

        def put_err(res):
            self.fail("can't put a row to storage")

        def trigger_ok4(res):
            #print 'put'
            import struct
            h = struct.pack('20b', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            g = ('GUID', h)
            self.row = {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0, 'COLUMN_GUID' : g }
            d = self.conn.put('pyunit_transactional_test_table', self.row)
            d.addCallback(put_ok)
            return d

        def trigger_func_table(tid, row, reason):
            self.table_trigger_called += 1

        def trigger_ok(res):
            #print 'put table trigger'
            d = self.conn.put_table_trigger('pyunit_transactional_test_table', True, trigger_func_table)
            d.addCallback(trigger_ok4)
            return d

        self.row_trigger_called = 0
        self.table_trigger_called = 0

        d = self.conn.put_row_trigger('pyunit_transactional_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123' }, trigger_func)
        d.addCallback(trigger_ok)
        return d

    def testTransactions(self):
        def get_close(result):
            #print 'get close ok'
            self.failUnless(result[0] == Storage.SUCCESS)

        def get_next(res):
            #print 'get next'
            result, self.row = res
            self.failUnless(result[0] == Storage.NO_MORE_ROWS)

            d = self.cursor.close()
            d.addCallback(get_close)
            return d

        def get_ok(res):
            #print 'get ok'
            result, self.cursor = res
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.cursor.get_next()
            d.addCallback(get_next)
            return d

        def rollback_ok(result):
            #print 'rollback ok'
            self.failUnless(result[0] == Storage.SUCCESS)

            d = self.conn.get('pyunit_transactional_test_table', { 'GUID' : self.guid })
            d.addCallback(get_ok)
            return d

        def put_ok(res):
            #print 'put ok'

            result, self.guid = res
            #print self.guid

            self.failUnless(result[0] == Storage.SUCCESS)
            d = self.conn.rollback()
            d.addCallback(rollback_ok)
            return d

        def begin_ok(result):
            self.failUnless(result[0] == Storage.SUCCESS)

            self.row = {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0 }
            d = self.conn.put('pyunit_transactional_test_table', self.row)
            d.addCallback(put_ok)
            return d

        from nox.ext.apps.tstorage import TransactionalConnection
        d = self.conn.begin(TransactionalConnection.EXCLUSIVE)
        d.addCallback(begin_ok)
        return d

    def testCreateSchemaCheck(self):

        def create_err(result):
            try:
                result.raiseException()
            except StorageException, e:
                self.failUnless(e.code == Storage.EXISTING_TABLE)

        def create_ok(result):
            self.fail("create_table with different schema should fail")

        def create_2(result):
            self.failUnless(result[0] == Storage.SUCCESS)
            d = self.conn.create_table("pyunit_transactional_test_table",
                                       {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float, "COLUMN_GUID" : tuple}, (('index_1', ("COLUMN_INT", "COLUMN_FLOAT")),), 0)
            d.addCallbacks(create_ok, create_err)
            return d

        d = self.conn.create_table("pyunit_transactional_test_table",
                                       {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float, "COLUMN_GUID" : tuple}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),), 0)
        d.addCallbacks(create_2)
        return d

def suite(ctxt):
    suite = pyunit.TestSuite()
    #suite.addTest(TransactionalStorageTestCase("testCreateGet", ctxt))
    #suite.addTest(TransactionalStorageTestCase("testPutGetModifyGetRemove", ctxt))
    #suite.addTest(TransactionalStorageTestCase("testTransactions", ctxt))
    #suite.addTest(TransactionalStorageTestCase("testCreateSchemaCheck", ctxt))

    # Meta tables

    return suite
