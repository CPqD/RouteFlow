/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

%module "nox.netapps.pycswitchstats"

%{
#include "cswitchstats_proxy.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"

%include "common-defs.i"
%include "cswitchstats_proxy.hh"

%pythoncode
%{
from nox.lib.netinet import netinet
from nox.lib.core import Component

class pycswitchstats(Component):
    """
    Provide interface to cswitchstats from python
    """  
    def __init__(self, ctxt):
        self.csstats = cswitchstats_proxy(ctxt)

    def configure(self, configuration):
        self.csstats.configure(configuration)

    def install(self):
        pass

    def getInterface(self):
        return str(pycswitchstats)

    def get_global_conn_p_s(self):
        return self.csstats.get_global_conn_p_s()

    def get_switch_conn_p_s(self, dpid):
        if type(dpid) == long:
            dpid = netinet.create_datapathid_from_host(dpid) 
        return self.csstats.get_switch_conn_p_s(dpid)

    def get_loc_conn_p_s(self, dpid, port):
        return self.csstats.get_loc_conn_p_s(dpid, port)


def getFactory():
      class Factory():
          def instance(self, context):
              return pycswitchstats(context)

      return Factory()
%}
