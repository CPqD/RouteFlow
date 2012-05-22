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

from nox.lib.core import Component
from nox.netapps.tests import unittest
import httplib
import re

from nox.webapps.webserviceclient.async import AsyncNOXWSClient, AsyncNOXWSResponse, DefaultLogin, FailureExceptionLogin

pyunit = __import__('unittest')

class WebserviceTestCase(unittest.NoxTestCase):

    def configure(self, configuration):
        pass

    def install(self):
        lmgr = DefaultLogin(backup=FailureExceptionLogin())
        self.wsc = AsyncNOXWSClient("127.0.0.1", 443, https=True, loginmgr=lmgr)

    def getInterface(self):
        return str(WebserviceTestCase)

    def _response_ok(self, r):
        self.failUnless(isinstance(r, AsyncNOXWSResponse), "Response is not an instance of AsyncNOXWSResponse")
        body = r.getExpectedBody(httplib.OK, "text/plain")
        match = re.search("GET /ws.v1/doc", body)
        self.failIf(match == None, "Did not find 'GET /ws.v1/doc' request documentation in the response.")

    def _response_err(self, e):
        e.raiseException()

    def run_test(self):
        d = self.wsc.get("/ws.v1/doc")
        d.addCallback(self._response_ok)
        d.addErrback(self._response_err)
        return d

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(WebserviceTestCase("run_test", ctxt))
    return suite
