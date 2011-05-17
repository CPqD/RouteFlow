#
# Copyright 2009 (C) Nicira, Inc.
#

import logging

from nox.lib.core import Component
from nox.netapps.data.pydatacache import PyData_cache, Principal_delete_event

lg = logging.getLogger('nox.netapps.data.datacache')

class DataCache(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.cache = PyData_cache(ctxt)
        Principal_delete_event.register_event_converter(self.ctxt)

    def getInterface(self):
        return str(DataCache)

    def configure(self, config):
        self.cache.configure(config)

    def get_authenticated_id(self, ptype):
        return self.cache.get_authenticated_id(ptype)

    def get_unauthenticated_id(self, ptype):
        return self.cache.get_unauthenticated_id(ptype)

    def get_unknown_id(self, ptype):
        return self.cache.get_unknown_id(ptype)

    def get_authenticated_name(self):
        return self.cache.get_authenticated_name()

    def get_unauthenticated_name(self):
        return self.cache.get_unauthenticated_name()

    def get_unknown_name(self):
        return self.cache.get_unknown_name()

    def get_name(self, ptype, id):
        return self.cache.get_name(ptype, id)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return DataCache(ctxt)

    return Factory()
