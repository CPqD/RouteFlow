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
#

# A couple of example tests:
#
# - Get all the flows on the switch with dpid 002320dab661:
#       PUT /ws.v1/nox/flowfetcher_test
#   with body
#       {"dpid": "0x002320dab661"}
#   Note the quoting of the dpid, because JSON doesn't support hexadecimal
#   constants.  How irritating.
#
# - Get all the flows on the switch with dpid 002320dab661 that have in_port 0:
#       PUT /ws.v1/nox/flowfetcher_test
#   with body
#       {"dpid": "0x002320dab661", "match": {"in_port": 0}}
#
# Note that the results are printed on the NOX console, not returned to the
# web browser.  (Sorry, I was lazy.)

import logging
import simplejson

from nox.lib.core import *

from nox.webapps.webservice  import webservice

from nox.lib.config   import version
from nox.lib.directory import Directory, DirectoryException
from nox.lib.netinet.netinet import datapathid

from nox.webapps.webservice.webservice import json_parse_message_body

from nox.netapps.flow_fetcher.pyflow_fetcher import flow_fetcher_app

from twisted.internet import defer
from twisted.python.failure import Failure

lg = logging.getLogger('pyflowfetcher_tests_ws')

def report_results(ff):
    status = ff.get_status()
    print "fetch status =", status
    if status == 0:
        print "flows =", ff.get_flows()

class pyflowfetcher_tests_ws(Component):
    """Web service interface info about nox"""

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def err(self, failure, request, fn_name, msg):
        lg.error('%s: %s' % (fn_name, str(failure)))
        if isinstance(failure.value, DirectoryException) \
                and (failure.value.code == DirectoryException.COMMUNICATION_ERROR \
                or failure.value.code == DirectoryException.REMOTE_ERROR):
            msg = failure.value.message
        return webservice.internalError(request, msg)

    def _test_flow_fetcher(self, request, arg):
        try:
            flow_stats_request = json_parse_message_body(request)
            dpid = datapathid.from_host(long(flow_stats_request['dpid'], 16))
            ff = self.ffa.fetch(dpid, flow_stats_request,
                                lambda: report_results(ff))
        except Exception, e:
            return self.err(Failure(), request, "_test_flow_fetcher",
                            "Could not request flows.")

    def install(self):

        self.ffa = self.resolve(flow_fetcher_app)

        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request

        # /ws.v1/nox
        noxpath = ( webservice.WSPathStaticString("nox"), )

        # /ws.v1/nox/flowfetcher_test
        noxfftpath = noxpath + ( webservice.WSPathStaticString("flowfetcher_test"), )
        reg(self._test_flow_fetcher, "PUT", noxfftpath,
            """Invoke Python flowfetcher test""")

    def getInterface(self):
        return str(pyflowfetcher_tests_ws)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return pyflowfetcher_tests_ws(ctxt)

    return Factory()
