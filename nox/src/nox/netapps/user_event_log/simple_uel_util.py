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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PUPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.

from nox.lib.core     import Component
from nox.lib.netinet.netinet import *
from nox.netapps.user_event_log.pyuser_event_log import pyuser_event_log,LogEntry
from twisted.internet.defer import Deferred
import time
import sys


class SimpleUELUtil(Component):
    """A simple class for testing UI network event log functionality"""

    def configure(self,configuration):
      pass

    def install(self):
      self.total_count = 0
      self.uel = self.resolve(pyuser_event_log) 
      self.post_callback(1,self.do_log)

    def getInterface(self):
        return str(SimpleUELUtil)
      
    # make this function generate whatever type of log message you need
    # to test the UI.  By using the sh,dh,su,du,sl,dl params you can 
    # avoid having to actually create bindings in bindings storage
    # Note: now that we use id's instead of names, you have to know
    # a uid corresponding to the correct principal types, otherwise
    # the name will show up as 'unknown' in the UI. 
    def do_log(self):
      self.uel.log("uel test",LogEntry.INFO,
                    "%s : {sh} is in trouble" % self.total_count, 
                    sh=5) 
      self.post_callback(1,self.do_log)
      self.total_count += 1

  
def getFactory():
        class Factory():
            def instance(self, context):
                return SimpleUELUtil(context)

        return Factory()
