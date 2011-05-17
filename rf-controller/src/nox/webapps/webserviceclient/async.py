"""An asynchoronous version of NOXWSClient"""

from simple import *
from twisted.internet import defer
from twisted.web.client import HTTPClientFactory
import re
import twisted
import httplib

class AsyncNOXWSResponse(NOXWSResponse):
    def __init__(self, status, reason, version, headers, body):
        self.status = status
        self.reason = reason
        self.version = version
        self._headers = headers
        self._body = body
        self._read = 0

    def read(self, amt=None):
        if amt == None:
            self._read = len(self._body)
            return self._body
        else:
            l = len(self._body) - self._read
            if amt < l:
                l = amt
            i = self._read
            self._read = self._read + amt
            return body[i: i+l]


class AsyncNOXWSClient(NOXWSClient):
    def __init__(self, host, port=DEFAULT_NOXWS_PORT, https=False, loginmgr=None):
        self.host = host
        self.port = port
        self.https = https
        if loginmgr == None:
            loginmgr = DefaultLogin()
        self.loginmgr = loginmgr
        self.cookies = {}
        self._deferred = None
        self._request = None
        self._url = None
        self._headers = None
        self._body = None

    def _capture_cookie(self):
        for s in self._request.response_headers.get("set-cookie", []):
            if s == None:
                return
            m = re.match("[ \t]*([^=]+)=([^;]*)", s)
            if m == None:
                return
            else:
                name = m.group(1)
                value = m.group(2)
                self.cookies[name] = value

    def _login_ok(self, m):
        self._capture_cookie()
        self.loginmgr.auth_complete()
        self._do_user_request()

    def _login_err(self, e):
        if e.check(twisted.web.error.Error, twisted.web.error.PageRedirect,
                   twisted.web.error.ErrorPage, twisted.web.error.NoResource,
                   twisted.web.error.ForbiddenResource):
            self._request_ok("")
            return
        self._deferred.errback(e)
        self._deferred = None

    def _request_ok(self, m):
        self._capture_cookie()
        r = AsyncNOXWSResponse(int(self._request.status),
                               self._request.message,
                               self._request.version,
                               self._request.response_headers,
                               m)
        self._deferred.callback(r)
        self._deferred = None


    def _request_err(self, e):
        if e.check(twisted.web.error.Error, twisted.web.error.PageRedirect,
                   twisted.web.error.ErrorPage, twisted.web.error.NoResource,
                   twisted.web.error.ForbiddenResource):
            if self._request.status == "401":
                self._capture_cookie()
                self._login()
            else:
                self._request_ok("")
            return
        self._deferred.errback(e)
        self._deferred = None

    def _full_url(self, abs_url_path):
        if self.https:
            url = "https://"
        else:
            url = "http://"
        return url + self.host + ":" + str(self.port) + abs_url_path

    def _make_request(self, method, url, headers, body):
        # This is a modified version of twisted.web.client.getPage() that
        # saves the factory object itself instead of the deferred so that
        # we can get access to the server status, etc.
        from twisted.internet import reactor
        self._request = HTTPClientFactory(url, method=method, headers=headers, postdata=body, cookies=self.cookies, agent="Python AsyncNOXWSClient", followRedirect=0)
        if self.https:
            from twisted.internet import ssl
            contextFactory = ssl.ClientContextFactory()
            reactor.connectSSL(self.host, self.port, self._request, contextFactory)
        else:
            reactor.connectTCP(self.host, self.port, self._request)

    def _do_login_request(self):
        headers = { "Content-Type" : "application/x-www-form-urlencoded" }
        user, password = self.loginmgr.get_credentials()
        body = urllib.urlencode({ "username": user,
                                  "password": password })
        self._make_request("POST", self._full_url("/ws.v1/login"), headers, body)
        self._request.deferred.addCallbacks(self._login_ok, self._login_err)

    def _do_user_request(self):
        self._make_request(self._method, self._url, self._headers, self._body)
        self._request.deferred.addCallbacks(self._request_ok, self._request_err)

    def _login(self):
        self.loginmgr.start_auth()
        self._do_login_request()

    def request(self, method, url, headers=None, body=""):
        if self._deferred != None:
            raise NOXWSError("Request already in progress.")
        self._deferred = defer.Deferred()
        self._method = method
        self._url = self._full_url(url)
        if headers == None:
            self._headers = {}
        else:
            self._headers = headers
        self._body = body
        self._do_user_request()
        return self._deferred

if __name__ == "__main__":
    from twisted.internet import reactor
    def handle_ok(r):
        print "Success Response\n" + "=" * 78 + "\n"
        print r.status, r.reason, "\n"
        for k, l in r.getHeaders().items():
            for i in l:
                print k + ": " + i
        print "-" * 78
        print r.getBody()
    def handle_err(e):
        print "Error Response\n" + "=" * 78 + "\n", str(e)
    lm = PersistentLogin()
    c = AsyncNOXWSClient("127.0.0.1", loginmgr=lm)
    d = c.get("/ws.v1/doc")
    d.addCallbacks(handle_ok, handle_err)
    reactor.run()
