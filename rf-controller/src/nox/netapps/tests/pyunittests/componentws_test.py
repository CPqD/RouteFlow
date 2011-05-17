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

class ComponentWsTestCase(unittest.NoxTestCase):
    version_search = '(\d\.)*\d'
    version_example = '1.2.3'
    uptime_example = '1234'
    name_search = '(.+\.)*.+'
    name_example = 'abc.xyz'


    def configure(self, configuration):
        pass

    def install(self):
        lmgr = DefaultLogin(backup=FailureExceptionLogin())
        self.wsc = AsyncNOXWSClient("127.0.0.1", 443, https=True, loginmgr=lmgr)

    def getInterface(self):
        return str(ComponentWsTestCase)

    def _response_err(self, e):
        e.raiseException()

    def check_version(self):
        d = self.wsc.get("/ws.v1/nox/version")
        d.addCallback(self._version_ok)
        d.addErrback(self._response_err)
        return d

    def _version_ok(self, r):
        self.failUnless(isinstance(r, AsyncNOXWSResponse), "Response is not an instance of AsyncNOXWSResponse")
        body = r.getExpectedBody(httplib.OK, "application/json")
        val = eval(body)
        self.failIf(type(val) != str, "Version '%s' is not a string." % body)
        match = re.search(self.version_search, val)
        self.failIf(match == None, "Version '%s' did not match expected '%s' format." % (body, self.version_example))

    def check_uptime(self):
        d = self.wsc.get("/ws.v1/nox/uptime")
        d.addCallback(self._uptime_ok)
        d.addErrback(self._response_err)
        return d

    def _uptime_ok(self, r):
        self.failUnless(isinstance(r, AsyncNOXWSResponse), "Response is not an instance of AsyncNOXWSResponse")
        body = r.getExpectedBody(httplib.OK, "application/json")
        val = eval(body)
        self.failIf(type(val) != int, "Uptime '%s' is non-integral." % body)
        self.failIf(val < 0, "Uptime '%s' is negative." % body)

    def check_components(self):
        d = self.wsc.get("/ws.v1/nox/components")
        d.addCallback(self._components_ok)
        d.addErrback(self._response_err)
        return d

    def _components_ok(self, r):
        self.failUnless(isinstance(r, AsyncNOXWSResponse), "Response is not an instance of AsyncNOXWSResponse")
        body = r.getExpectedBody(httplib.OK, "application/json")
        d =  eval(body)
        for c in d['items']:
            n = c['name']
            match = re.search(self.name_search, n)
            self.failIf(match == None, "Component name '%s' did not match expected '%s' format." % (n,self.name_example))
            v = c['version']
            match = re.search('(\d\.)*\d', v)
            self.failIf(match == None, "Component %s: version '%s' did not match expected '%s' format." % (n,v,self.version_example))
            u = c['uptime']
            self.failIf(type(u) != int, "Component %s: uptime '%s' non-integral." % (n,u))
            self.failIf(u < 0, "Component %s: uptime '%s' negative." % (n,u))

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(ComponentWsTestCase("check_version", ctxt))
    suite.addTest(ComponentWsTestCase("check_uptime", ctxt))
    suite.addTest(ComponentWsTestCase("check_components", ctxt))
    return suite
