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
# ----------------------------------------------------------------------

import logging
from nox.lib.core import *

import os.path

lg = logging.getLogger('cpustats')

class cpustats(Component):
     
    proc_stats   = "/proc/stat"
    POLL_PERIOD  = 5 # poll cpu stats every 5 seconds

    def update_cpu_stats(self):
        fd = open(cpustats.proc_stats)
        if not fd:
            lg.error("unable to read "+cpustats.proc_stats+" disabling cpustats")
            return
        line = fd.readline().split(' ')[2:10]
        if len(line) != 8:
            print line
            lg.error("invalid "+cpustats.proc_stats+" format. disabling cpustats")
            return

        # convert values to ints
        line = map(lambda x: int(x), line) 
        # get change in values over last time period 
        delta_vals = map(lambda a,b : a-b, line, self.old_vals)
        # Get total usage
        tot        =  float(reduce(lambda a,b : a+b, delta_vals))

        self.last_user_time = 100.0 * (float(delta_vals[0] + delta_vals[1]) / tot)
        self.last_sys_time  = 100.0 * (float(delta_vals[2]) / tot)
        self.last_io_time   = 100.0 * (float(delta_vals[4]) / tot)
        self.last_irq_time  = 100.0 * (float(delta_vals[5]) / tot)

        self.old_vals = line
        self.post_callback(cpustats.POLL_PERIOD, self.update_cpu_stats)

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.old_vals = [0,0,0,0,0,0,0,0] 
        self.last_user_time = 0.
        self.last_sys_time  = 0.
        self.last_io_time   = 0.
        self.last_irq_time  = 0.

    def install(self):

        if not os.path.exists(cpustats.proc_stats):
            lg.error("unable to read "+cpustats.proc_stats+" disabling cpustats")
            return

        self.post_callback(cpustats.POLL_PERIOD, self.update_cpu_stats)
            

    def getInterface(self):
        return str(cpustats)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return cpustats(ctxt)

    return Factory()
