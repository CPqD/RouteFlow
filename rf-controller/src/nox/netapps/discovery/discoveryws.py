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
# =====================================================================
#
# 
#
from nox.netapps.discovery.discovery import discovery
from nox.netapps.switchstats.switchstats import switchstats

from nox.webapps.webservice      import webservice

from nox.webapps.webservice.webservice import json_parse_message_body
from nox.webapps.webservice.webservice import NOT_DONE_YET 

from nox.lib.core import *

import simplejson
import time

#
# Verifies that pc is a valid datapath name 
#
class WSPathExistingSwitchName(webservice.WSPathComponent):
    def __init__(self, switchstats):
        webservice.WSPathComponent.__init__(self)
        self._switchstats = switchstats

    def __str__(self):
        return "<switch name>"

    def extract(self, pc, data):
        if pc == None:
            return webservice.WSPathExtractResult(error="End of requested URI")
        try:
            if self._switchstats.get_dp(pc) == None:
                e = "switch '%s' is unknown" % pc
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid switch name %s" % str(pc)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(pc)

class discoveryws(Component):
    """Web service for accessing topology information"""

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def gen_switch_name_cb(self, dp, stats):
        def f(res):
            if len(res) > 0:
                self.bind_dp_switch_name(dp, res[0][0]) 
                stats['name'] = res[0][0]
        f.dp    = dp        
        f.stats = stats 
        return f        

    def _get_links(self, request, arg):
        response = {}
        response['identifier'] = 'name'
        response['items'] = []
        for tuple in self._discovery.adjacency_list.keys():
            link = {}
            link['ts']    = time.time() - self._discovery.adjacency_list[tuple]
            link['dp1']   = self._switchstats.get_switch_name(tuple[0])
            link['dp2']   = self._switchstats.get_switch_name(tuple[2])
            link['port1'] = self._switchstats.get_dp_port_name(tuple[0], tuple[1])
            link['port2'] = self._switchstats.get_dp_port_name(tuple[2], tuple[3])
            response['items'].append(link)
        return simplejson.dumps(response)    

    def _get_switch_links(self, request, arg):
        response = {}
        response['identifier'] = 'name'
        response['items'] = []

        dpid = self._switchstats.get_dp(arg['<switch name>'])
        for tuple in self._discovery.adjacency_list.keys():
            if tuple[0] == dpid or tuple[2] == dpid:
                link = {}
                link['ts']    = time.time() - self._discovery.adjacency_list[tuple]
                link['dp1']   = self._switchstats.get_switch_name(tuple[0])
                link['dp2']   = self._switchstats.get_switch_name(tuple[2])
                link['port1'] = self._switchstats.get_dp_port_name(tuple[0], tuple[1])
                link['port2'] = self._switchstats.get_dp_port_name(tuple[2], tuple[3])
                response['items'].append(link)
        return simplejson.dumps(response)    

    def install(self):
        self._discovery   = self.resolve(discovery)
        self._switchstats = self.resolve(switchstats)
        
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request

        # /ws.v1/topology
        topologypath = ( webservice.WSPathStaticString("topology"), )

        # /ws.v1/topology/links
        linkspath =  topologypath + \
                     ( webservice.WSPathStaticString("links"), ) 
        reg(self._get_links, "GET", linkspath,
            """Get list of links.""")

        # /ws.v1/topology/switch/    
        switchpath = topologypath + \
                     ( webservice.WSPathStaticString("switch"), ) 

        # /ws.v1/topology/switch/<switch name>
        switchnamepath = switchpath + \
                        (WSPathExistingSwitchName(self._switchstats) ,)

        # /ws.v1/topology/switch/<switch name>/links
        switchlinkspath =  switchnamepath + \
                     ( webservice.WSPathStaticString("links"), ) 
        reg(self._get_switch_links, "GET", switchlinkspath,
            """Get list of links.""")


    def getInterface(self):
        return str(discoveryws)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return discoveryws(ctxt)

    return Factory()
