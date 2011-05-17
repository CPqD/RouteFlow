/* Copyright 2009 (C) Nicira, Inc.
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

%module "nox.netapps.pyswitch_management"

%{
#include "switch_management_proxy.hh"
#include "pyrt/pycontext.hh"
using namespace std;
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/datapathid.i"
%import "netinet/netinet.i"

%include "std_string.i"
%include "utf8_string.i"
%include "std_list.i"
%include "common-defs.i"
%include "switch_management_proxy.hh"

%template(strlist) std::list<std::string>;

%pythoncode
%{
from nox.lib.netinet import netinet
from nox.lib.core import Component

class pyswitch_management(Component):
    """
    Provide interface to switch_management from python
    """  
    def __init__(self, ctxt):
        self.smp = Switch_management_proxy(ctxt)

    def configure(self, configuration):
        self.smp.configure(configuration)

    def install(self):
        pass

    def getInterface(self):
        return str(pyswitch_management)

    def get_mgmt_id(self, datapathid):
        return self.smp.get_mgmt_id(datapathid)

    def switch_mgr_is_active(self, mgmt_id):
        return self.smp.switch_mgr_is_active(mgmt_id)

    def get_port_name(self, mgmtid, portid):
        return self.smp.get_port_name(mgmtid, portid)
    
    def mgmtid_to_system_uuid(self, mgmtid):
        return self.smp.mgmtid_to_system_uuid(mgmtid)

    def set_all_entries(self, mgmtid, key, val_tuple):
        s = strlist()
        for v in val_tuple:
            s.push_back(v)
        return self.smp.set_all_entries(mgmtid, key, s)


def getFactory():
      class Factory():
          def instance(self, context):
              return pyswitch_management(context)

      return Factory()
%}
