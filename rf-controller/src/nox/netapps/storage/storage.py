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

__all__ = ['Storage', 'StorageException']

class Storage:
    """Non-transactional multi key--value pair storage.

    Note all methods return a Twisted deferred object.  In other
    words, while method descriptions say 'Return value', the
    description actually means the value is passed to the deferred's
    callback/errback.
    """

    # Operations return their Result as a 2-tuple consisting of a code
    # (see the int constants defined below) and a brief error
    # description as a string, if an error occurred.
    
    SUCCESS = 0                 # OK
    NO_MORE_ROWS = 1            # Iteration complete.
    CONCURRENT_MODIFICATION = 2 # Respective indices/rows modified in the
                                # middle of an operation.
    EXISTING_TABLE = 3          # Table already exists.
    NONEXISTING_TABLE = 4       # Table doesn't exist.
    INVALID_ROW_OR_QUERY = 5    # Invalid row or query.
    INVALID_CONTEXT = 6         # Context and query do not match.
    CONNECTION_ERROR = 7        # Network connectivity error.
    UNKNOWN_ERROR = 8           # Uncategorized error.

    # Invoked triggers include the original reason for invocation.
    INSERT = 0                  # Row was created
    MODIFY = 1                  # Row was modified
    REMOVE = 2                  # Row was removed

    # NOX_SCHEMA_META, NOX_SCHEMA_TABLE, and NOX_SCHEMA_INDEX tables
    # contain the database schemas for persistent and non-persistent
    # storages.    The tables use the following constants.
    #
    # TODO: add the meta table schemas here

    # Column types
    COLUMN_INT, COLUMN_TEXT, COLUMN_DOUBLE, COLUMN_GUID = [0, 1, 2, 3]

    # Table types
    PERSISTENT, NONPERSISTENT = [0, 1]

    def create_table(self, table, columns, indices):
        """Create a table.

        Returns Result object.
        
        Arguments:

        table      -- the case sensitive table name.
        columns    -- a dictionary: key = column name, value = a Python
                      string type, long/int type object, float type
                      object or buffer type object. Python type object
                      determines the column type.
        indices    -- a tuple of index definitions.  Each index
                      definition is a 2-tuple: a column name followed
                      by index column names as a tuple.
        """
        pass

    def drop_table(self, table):
        """Drop a table.

        Returns Result object.

        Arguments:

        table      -- the case sensitive table name.
        """
        pass

    def get(self, table, query):
        """Get a row.
        
        Returns a 3-tuple consisting of Result, Context, Row
        (represented by dictionaries consisting of key-value pairs)
        objects. Note, if no row can't be found, Result object
        contains status code of NO_MORE_ROWS.

        Arguments:

        table    -- the table name.
        query    -- a dictionary with column names and their corresponding
                    key values.  column names must match to an index.  if 
                    no query columns are defined, table contents are returned.
        """
        pass

    def get_next(self, context):
        """Get next row.

        Returns a 3-tuple consisting of Result, Context, Row
        (represented by dictionaries consisting of key-value pairs)
        objects. Note, if no row can't be found, Result object
        contains status code of NO_MORE_ROWS.
        """
        pass

    def put_row_trigger(self, context, trigger):
        """Put a trigger.
        
        Return a 2-tuple consisting of Result and a trigger id (if
        trigger was succesfully created).

        context    -- a context instance returned by earlier get() call.
        trigger    -- trigger callback.  expected function signature: 
                      f(trigger id, row, reason).
        """
        pass

    def put_table_trigger(self, table, sticky, trigger):
        """Put a table trigger.
        
        Return a 2-tuple consisting of Result and a trigger id (if
        trigger was succesfully created).

        table      -- the table name.
        sticky     -- whether the trigger is sticky (True) or not (False)
        trigger    -- trigger callback.  expected function signature: 
                      f(trigger id, row, reason).
        """
        pass

    def remove_trigger(self, trigger_id):
        """Remove a trigger.

        Returns Result object.

        Arguments:

        trigger_id -- trigger id of the trigger to remove.
        """
        pass

    def put(self, table, row):
        """Put a new row.
        
        Returns 2-tuple consisting of a Result object and the GUID of
        the new row.

        Arguments:

        table    -- the table name.
        row      -- a dictionary with every column name and their
                    corresponding values.  a new random 20 byte guid
                    will be generated.
        """
        pass

    def modify(self, context, row):
        """Modify an existing row.

        Returns 2-tuple consisting of a Result object and a new
        Context object.

        Arguments:

        context  -- context instance returned by an earlier get() call.
        row      -- a dictionary with every column name and their corresponding
                    (possibly) modified values.
        """
        pass

    def remove(self, context):
        """Remove an existing row.  Note that the context is invalid after the
        call, and a new one should be retrieved with get() if
        necessary.

        Returns Result object.

        Arguments:

        context  -- context instance returned by an earlier get() call.
        """
        pass

class StorageException(Exception):
    """Storage exception."""
    def __init__(self, code, message):
        self.code = code
        self.message = message

    def __str__(self):
        return repr(str(self.code) + ": " + self.message)

def getFactory():
    class Factory():
        def instance(self, context):
            from nox.netapps.storage.storage import Storage
            from nox.netapps.storage.pystorage import PyStorage
            from nox.netapps.storage.util import create_method
            
            class PyStorageProxy(Storage):
                """A proxy for the C++ based Python bindings to
                avoid instantiating Twisted Failure on the C++ side."""  
                def __init__(self, ctxt):
                    self.impl = impl = PyStorage(ctxt)
                    self.create_table = create_method(impl.create_table)
                    self.drop_table = create_method(impl.drop_table)
                    self.get = create_method(impl.get)
                    self.get_next = create_method(impl.get_next)
                    self.put = create_method(impl.put)
                    self.modify = create_method(impl.modify)
                    self.remove = create_method(impl.remove)
                    self.put_row_trigger = \
                        create_method(impl.put_row_trigger)
                    self.put_table_trigger = \
                        create_method(impl.put_table_trigger)
                    self.remove_trigger = create_method(impl.remove_trigger)

                def configure(self, configuration):
                    self.impl.configure(configuration)

                def install(self):
                    pass

                def getInterface(self):
                    return str(Storage)
                        
            return PyStorageProxy(context)

    return Factory()
