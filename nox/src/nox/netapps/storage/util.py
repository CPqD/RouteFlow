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
from twisted.internet import defer
from twisted.python import failure

__all__ = ['create_method']

def create_callback(deferred):
    from twisted.python import failure
    from storage import Storage, StorageException

    def callback(*args, **kw):
        result = args[0]
        if result[0] == Storage.SUCCESS or \
                result[0] == Storage.NO_MORE_ROWS:
            if len(args) == 1:
                deferred.callback(result)
            else:
                deferred.callback(args)
        else:
            deferred.errback(StorageException(result[0], result[1]))

    return callback
        
def create_method(impl, filter=None):
    from twisted.internet import defer

    def method(*args, **kw):
        d = defer.Deferred()
        if filter:
            d.addCallback(filter)
        impl(*(args + (create_callback(d), )))
        return d

    return method
