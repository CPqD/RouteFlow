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
# Trivial example using reactor timer method to countdown from three

from nox.lib.core     import *
import logging

logger = logging.getLogger('nox.coreapps.examples.countdown')
numbers = ["one","two","three"]
index = 0

class countdown(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        
    def install(self):
        # call every second
        self.post_callback(1, lambda : self.count_down())

    def getInterface(self):
        return str(countdown)

    def count_down(self):

        global index
        # No, this isn't mispelled:.  If you're curious, see Farscape
        # episode 1.17
        logger.debug("%s %s" % (numbers[index], 'mippippi'))
        index+=1
        if index < len(numbers):
            self.post_callback(1, lambda : self.count_down())

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return countdown(ctxt)

    return Factory()
