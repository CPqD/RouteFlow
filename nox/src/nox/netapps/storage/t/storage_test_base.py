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
import logging

from nox.coreapps.testharness.testdefs import *
from nox.lib.core import Component, CONTINUE
from nox.netapps.storage.storage import StorageException, Storage

log = logging.getLogger('storage test')

class StorageTestBase(Component):

    def testAddthenDrop(self):
        log.debug("testAddthenDrop")
        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def create_ok(res):
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(create_ok)
        return d

    def testAddthenPut(self):
        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def put_ok(result):
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def put(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0 })
            d.addCallback(put_ok)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(put)
        return d

    def testAddthenPutWrongTable(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            nox_test_fail(0)
            
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def put_err(result):
            if result.type != StorageException:
                result.raiseException()
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def put_ok(result):
            nox_test_fail(False)

        def put(res):
            d = self.impl.put('pyfoo', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0 })
            d.addCallback(put_ok)
            d.addErrback(put_err)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(put)
        return d

    def testAddthenPutthenGet(self):
        """Create table, put a row (with an unicode string column value), get
           it and drop the table."""

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def get_ok(res):
            result, ctxt, row = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            nox_test_assert(row['COLUMN_INT'] == 1234)
            nox_test_assert(row['COLUMN_STR'] == u'string123\u1234foo')
            nox_test_assert(row['COLUMN_FLOAT'] == 3456.)

            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def get(result):
            d = self.impl.get('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : u'string123\u1234foo'})
            d.addCallback(get_ok)
            return d

        def put(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : u'string123\u1234foo', 'COLUMN_FLOAT' : 3456.0 })
            d.addCallback(get)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(put)
        return d

    def testAddthenPutthenGetNextUsingIndex(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def get_next(res):
            result, ctxt, row = res

            if result[0] == Storage.NO_MORE_ROWS:
                d = self.impl.drop_table('pyunit_test_table')
                nox_test_assert(self.rows == 2)
                d.addCallback(dropped_ok)
                d.addErrback(dropped_err)
                return d
            else:
                self.rows += 1
                nox_test_assert(self.rows <= 2)
                nox_test_assert(row['COLUMN_INT'] == 1234)
                nox_test_assert(row['COLUMN_STR'] == 'string123')
                nox_test_assert(row['COLUMN_FLOAT'] == 3456.0 or
                                row['COLUMN_FLOAT'] == 7890.0)
                d = self.impl.get_next(ctxt)
                d.addCallback(get_next)
                return d

        def get(result):
            self.rows = 0
            d = self.impl.get('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123'})
            d.addCallback(get_next)
            return d

        def put2(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 7890.0 })
            d.addCallback(get)
            return d

        def put1(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0 })
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(put1)
        return d

    def testAddthenPutthenGetNextAll(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def get_next(res):
            result, ctxt, row = res

            if result[0] == Storage.NO_MORE_ROWS:
                d = self.impl.drop_table('pyunit_test_table')
                nox_test_assert(self.rows == 2)
                d.addCallback(dropped_ok)
                d.addErrback(dropped_err)
                return d
            else:
                self.rows += 1
                nox_test_assert(self.rows <= 2)
                nox_test_assert(row['COLUMN_INT'] == 1234)
                nox_test_assert(row['COLUMN_STR'] == 'string123')
                nox_test_assert(row['COLUMN_FLOAT'] == 3456.0 or
                                row['COLUMN_FLOAT'] == 7890.0)
                d = self.impl.get_next(ctxt)
                d.addCallback(get_next)
                return d

        def get(result):
            self.rows = 0
            d = self.impl.get('pyunit_test_table', {});
            d.addCallback(get_next)
            return d

        def put2(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 7890.0 })
            d.addCallback(get)
            return d

        def put1(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_FLOAT' : 3456.0 })
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT" : int, "COLUMN_STR" : str, "COLUMN_FLOAT" : float}, (('index_1', ("COLUMN_INT", "COLUMN_STR")),))
        d.addCallback(put1)
        return d

    def testAddthenPutthenGetDoubleIndexCheck(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def done():
            nox_test_assert(self.count == 1)
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def count_next(res):
            result, ctxt, row = res
            if (result[0] == Storage.SUCCESS):
                self.count += 1
                d = self.impl.get_next(ctxt)
                d.addCallback(count_next)
                return d

            if (result[0] == Storage.NO_MORE_ROWS):
                return done()

            nox_test_assert(False)

        def count_all(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            self.count = 0
            d = self.impl.get('pyunit_test_table', {})
            d.addCallback(count_next)
            return d

        def get_next2(res):
            result, ctxt, row = res
            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['COLUMN_INT2'] == 1234)
            nox_test_assert(row['COLUMN_STR'] == 'string123')
            nox_test_assert(row['COLUMN_INT1'] == 7890)
            nox_test_assert(row['COLUMN_INT3'] == 1234)

            d = self.impl.remove(ctxt)
            d.addCallback(count_all)
            return d

        def get_next1(res):
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['COLUMN_INT1'] == 1234)
            nox_test_assert(row['COLUMN_STR'] == 'string123')
            nox_test_assert(row['COLUMN_INT2'] == 3456)
            nox_test_assert(row['COLUMN_INT3'] == 1234)

            d = self.impl.get('pyunit_test_table', { 'COLUMN_INT2' : 1234,
                                                     'COLUMN_STR' : 'string123' });
            d.addCallback(get_next2)
            return d

        def get1(result):
            d = self.impl.get('pyunit_test_table', { 'COLUMN_INT1' : 1234,
                                                     'COLUMN_STR' : 'string123' });
            d.addCallback(get_next1)
            return d

        def put2(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT1' : 7890, 'COLUMN_STR' : 'string123', 'COLUMN_INT2' : 1234, 'COLUMN_INT3': 1234} )
            d.addCallback(get1)
            return d

        def put1(res):
            d = self.impl.put('pyunit_test_table', {'COLUMN_INT1' : 1234, 'COLUMN_STR' : 'string123', 'COLUMN_INT2' : 3456, 'COLUMN_INT3': 1234} )
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"COLUMN_INT1" : int, "COLUMN_STR" : str,
                                    "COLUMN_INT2" : int, "COLUMN_INT3" : int },
                                   (('index_1', ("COLUMN_INT1", "COLUMN_STR")),
                                    ('index_2', ("COLUMN_INT2", "COLUMN_STR")),
                                    ('index_3', ("COLUMN_INT3", "COLUMN_STR"))))
        d.addCallback(put1)
        return d

    def testMultiIndex(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def done():
            nox_test_assert(self.count == 1)
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def count_next(res):
            result, ctxt, row = res
            if (result[0] == Storage.SUCCESS):
                self.count += 1
                d = self.impl.get_next(ctxt)
                d.addCallback(count_next)
                return d

            if (result[0] == Storage.NO_MORE_ROWS):
                return done()

            nox_test_assert(False)

        def count(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            self.count = 0
            d = self.impl.get('pyunit_test_table', {})
            d.addCallback(count_next)
            return d

        def remove(res):
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['DPID'] == 6)
            nox_test_assert(row['PORT'] == 7)
            nox_test_assert(row['DL_ADDR'] == 8)
            nox_test_assert(row['NW_ADDR'] == 200)
            nox_test_assert(row['ID'] == 5)

            d = self.impl.remove(ctxt)
            d.addCallback(count)
            return d

        def get(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table',
                              { 'DPID' : 6, 'PORT' : 7, 'DL_ADDR' : 8,
                                'NW_ADDR' : 200, 'ID' : 5 });
            d.addCallback(remove)
            return d

        def put2(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'DPID' : 6, 'PORT' : 7, 'DL_ADDR' : 8, 'NW_ADDR': 200, 'ID' : 5} )

            d.addCallback(get)
            return d

        def put1(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'DPID' : 6, 'PORT' : 7, 'DL_ADDR' : 8, 'NW_ADDR': 0, 'ID' : 5} )
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"DPID" : int, "PORT" : int, "DL_ADDR" : int,
                                    "NW_ADDR" : int, "ID": int },
                                   (('INDEX_1', ("DPID", "PORT")),
                                    ('INDEX_2', ("DL_ADDR", )),
                                    ('INDEX_3', ("NW_ADDR", )),
                                    ('INDEX_4', ("DPID", "PORT", "DL_ADDR", "NW_ADDR")),
                                    ('INDEX_5', ("DPID", "PORT", "DL_ADDR", "NW_ADDR", "ID")),
                                    ('INDEX_6', ("ID", ))))
        d.addCallback(put1)
        return d

    def testMultiIndexModify(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def done():
            nox_test_assert(self.count == 1)
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def count_err(res):
            nox_test_assert(False)

        def count_next(res):
            result, ctxt, row = res
            if (result[0] == Storage.SUCCESS):
                self.count += 1
                d = self.impl.get_next(ctxt)
                d.addCallback(count_next)
                d.addErrback(count_err)
                return d

            if (result[0] == Storage.NO_MORE_ROWS):
                return done()

            nox_test_assert(False)

        def count(res):
            #log.debug('xxxxxx count xxxxxxxxxxx')
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            self.count = 0

            d = self.impl.get('pyunit_test_table', { 'DL_ADDR' : 8 })
            d.addCallback(count_next)
            d.addErrback(count_err)

            return d

        def get3(res):
            #log.debug('xxxxxx get3 xxxxxxxxxxx')
            result, ctxt = res

            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table', { 'NW_ADDR' : 200 });
            d.addCallback(count)
            return d

        def modify2(res):
            #log.debug('xxxxxx modify2 xxxxxxxxxxx')
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['DPID'] == 6)
            nox_test_assert(row['PORT'] == 7)
            nox_test_assert(row['DL_ADDR'] == 8)
            nox_test_assert(row['NW_ADDR'] == 201)
            nox_test_assert(row['ID'] == 5)

            row['NW_ADDR'] = 200

            d = self.impl.modify(ctxt, row)
            d.addCallback(get3)
            return d

        def get2(res):
            #log.debug( 'xxxxxx get2 xxxxxxxxxxx')
            result, ctxt = res

            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table', { 'NW_ADDR' : 201 });
            d.addCallback(modify2)
            return d

        def modify(res):
            #log.debug('xxxxxxxxxxxxxx modify xxxxxxxxxx')
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['DPID'] == 6)
            nox_test_assert(row['PORT'] == 7)
            nox_test_assert(row['DL_ADDR'] == 8)
            nox_test_assert(row['NW_ADDR'] == 200)
            nox_test_assert(row['ID'] == 5)

            row['NW_ADDR'] = 201

            d = self.impl.modify(ctxt, row)
            d.addCallback(get2)
            return d

        def get(res):
            #log.debug('xxxxxxxxxxxxxx getting xxxxxxxxxx')
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table',
                              { 'DPID' : 6, 'PORT' : 7, 'DL_ADDR' : 8,
                                'NW_ADDR' : 200, 'ID' : 5 });
            d.addCallback(modify)
            return d

        def put1(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'DPID' : 6, 'PORT' : 7, 'DL_ADDR' : 8, 'NW_ADDR': 200, 'ID' : 5} )
            d.addCallback(get)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"DPID" : int, "PORT" : int, "DL_ADDR" : int,
                                    "NW_ADDR" : int, "ID": int },
                                   (('INDEX_1', ("DPID", "PORT")),
                                    ('INDEX_2', ("DL_ADDR", )),
                                    ('INDEX_3', ("NW_ADDR", )),
                                    ('INDEX_4', ("DPID", "PORT", "DL_ADDR", "NW_ADDR")),
                                    ('INDEX_5', ("DPID", "PORT", "DL_ADDR", "NW_ADDR", "ID")),
                                    ('INDEX_6', ("ID", ))))
        d.addCallback(put1)
        return d

    def testRemoveMultipleRows(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def done():
            nox_test_assert(self.count == 1)
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def count_next(res):
            result, ctxt, row = res
            if (result[0] == Storage.SUCCESS):
                self.count += 1
                d = self.impl.get_next(ctxt)
                d.addCallback(count_next)
                return d

            if (result[0] == Storage.NO_MORE_ROWS):
                return done()

            nox_test_assert(False)

        def count(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            self.count = 0
            d = self.impl.get('pyunit_test_table', {})
            d.addCallback(count_next)
            return d

        def remove(res):
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['ID'] == 1)

            d = self.impl.remove(ctxt)
            d.addCallback(count)
            return d

        def get(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table', { 'ID' : 1})
            d.addCallback(remove)
            return d

        def put2(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'ID' : 1, 'NAME' : "bar"} )

            d.addCallback(get)
            return d

        def put1(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'ID' : 1, 'NAME' : "foo"} )
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"ID" : int, "NAME" : str},
                                   (('INDEX_1', ("ID",)),)  )
        d.addCallback(put1)
        return d


    def testGetRemoveSequence(self):

        def dropped_ok(res):
            pass

        def dropped_err(res):
            if res.type == StorageException:
                if res.value.code != 8:
                    # ignore not implemented error from Storage
                    res.raiseException()

        def done():
            nox_test_assert(self.count == 0)
            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def count_next(res):
            result, ctxt, row = res
            if (result[0] == Storage.SUCCESS):
                self.count += 1
                d = self.impl.get_next(ctxt)
                d.addCallback(count_next)
                return d

            if (result[0] == Storage.NO_MORE_ROWS):
                return done()

            nox_test_assert(False)

        def count(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            self.count = 0
            d = self.impl.get('pyunit_test_table', {})
            d.addCallback(count_next)
            return d

        def remove2(res):
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['ID'] == 1)

            d = self.impl.remove(ctxt)
            d.addCallback(count)
            return d

        def get2(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)
            d = self.impl.get('pyunit_test_table', { 'ID' : 1})
            d.addCallback(remove2)
            return d

        def remove1(res):
            result, ctxt, row = res

            nox_test_assert(result[0] == Storage.SUCCESS)
            nox_test_assert(row['ID'] == 1)

            d = self.impl.remove(ctxt)
            d.addCallback(get2)
            return d

        def get1(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.get('pyunit_test_table', { 'ID' : 1})
            d.addCallback(remove1)
            return d

        def put2(res):
            result, guid = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'ID' : 1, 'NAME' : "bar"} )

            d.addCallback(get1)
            return d

        def put1(res):
            result = res
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.put('pyunit_test_table',
                              {'ID' : 1, 'NAME' : "foo"} )
            d.addCallback(put2)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"ID" : int, "NAME" : str},
                                   (('INDEX_1', ("ID",)),
                                    ('INDEX_2', ("ID",))))
        d.addCallback(put1)
        return d

    def testCreateSchemaCheck(self):
        def dropped_ok(res):
            pass

        def dropped_err(res):
            pass

        def create_err(result):
            try:
                result.raiseException()
            except StorageException, e:
                nox_test_assert(e.code == Storage.EXISTING_TABLE)

            d = self.impl.drop_table('pyunit_test_table')
            d.addCallback(dropped_ok)
            d.addErrback(dropped_err)
            return d

        def create_ok(result):
            self.fail("create_table with different schema should fail")

        def create_3(result):
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.create_table("pyunit_test_table",
                                   {"ID" : int, "NAME" : str},
                                   (('INDEX_1', ("NAME",)),
                                    ('INDEX_2', ("ID",))))
            d.addCallback(create_ok)
            d.addErrback(create_err)
            return d

        def create_2(result):
            nox_test_assert(result[0] == Storage.SUCCESS)

            d = self.impl.create_table("pyunit_test_table",
                                   {"ID" : int, "NAME" : str},
                                   (('INDEX_1', ("ID",)),
                                    ('INDEX_2', ("ID",))))
            d.addCallback(create_3)
            return d

        d = self.impl.create_table("pyunit_test_table",
                                   {"ID" : int, "NAME" : str},
                                   (('INDEX_1', ("ID",)),
                                    ('INDEX_2', ("ID",))))
        d.addCallback(create_2)
        return d
