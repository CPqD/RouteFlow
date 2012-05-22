#!/usr/bin/env python

import logging
log = logging.getLogger("webserviceclient")

import sys
from getpass import getpass
from cmd import Cmd
import httplib
import urllib
import simplejson
import readline
from optparse import OptionParser
from os import path, environ, unlink
from tempfile import mkstemp
import subprocess
import socket
import types

DEFAULT_NOXWS_PORT = 8888
DEFAULT_NOXWS_USER = "admin"
DEFAULT_NOXWS_PASSWORD = "admin"
DEFAULT_MSGBODY_FILE = "~/.noxwsclient.msgbodies"
HISTORY_FILE = path.join(environ["HOME"], ".noxwsclient.history")

class NOXWSError(StandardError):
    pass

class NOXWSConnectError(NOXWSError):
    pass

class NOXWSLoginError(NOXWSError):
    pass

class NOXWSMsgTypeError(NOXWSError):
    pass

class NOXWStatusCodeError(NOXWSError):
    pass

class NOXWSLoginMgr:
    def __init__(self, user=None, password=None):
        self.user = user
        self.password = password

    def start_auth(self):
        """Called before beginning authentication."""

    def get_credentials(self):
        """Get login credentials for user.

        Returns a tuple of (user,password)"""
        raise ImplementationError("get_credentials must be implemented by a subclass.")

    def auth_complete(self):
        """Called after authentication has succeeded.

        The get_credentials method could be called an arbitrary number of
        times after the initial get_credentials call before the
        auth_complete method is called."""
        return  # Default is to do nothing.



class StdStreamPromptedLogin(NOXWSLoginMgr):
    """Always prompt on stdout/stdin for login credentials.

    This is the most secure since it doesn't keep password information
    around any longer than required.  However, it can be inconvenient to
    use."""
    def __init__(self, user=None, password=None):
        NOXWSLoginMgr.__init__(self, user, password)
        self._wipe_password()

    def start_auth(self):
        self.count = 0

    def _wipe_password(self):
        self.password = None
        # TBD: force a garbage collect here to remove password from
        # TBD: memory.  Is that really sufficient?

    def get_credentials(self):
        if self.password != None:
            self._wipe_password()
        if self.count > 0:
            self.user = None
        self.count = self.count + 1
        if self.user == None:
            sys.stdout.write("Username: ");
            sys.stdout.flush();
            self.user = sys.stdin.readline().strip();
        self.password = getpass()
        if self.password == "":
            self.user = None
            return self.get_credentials()
        return (self.user, self.password)

    def auth_complete(self):
        self._wipe_password()

class PersistentLogin(NOXWSLoginMgr):
    """On subsequent attempts use last credentials before prompting again."""
    def __init__(self, user=None, password=None, backup=None):
        NOXWSLoginMgr.__init__(self, user, password)
        if backup == None:
            backup = StdStreamPromptedLogin()
        self.backup = backup
        self.used_backup = False
        self.first = True

    def start_auth(self):
        self.used_backup = False
        self.first  = True

    def _get_from_backup(self):
        if self.used_backup == False:
            self.used_backup = True
            self.backup.user = self.user
            self.backup.password = self.password
            self.backup.start_auth()
        self.user, self.password = self.backup.get_credentials()

    def get_credentials(self):
        if not self.first:
            self.user = None
            self.password = None
        if self.user == None or self.password == None:
            self._get_from_backup()
        self.first = False
        return (self.user, self.password)

    def auth_complete(self):
        if self.used_backup:
            self.backup.auth_complete()


class DefaultLogin(PersistentLogin):
    """Attempt a default username/password login before prompting."""
    def __init__(self, user=DEFAULT_NOXWS_USER, password=DEFAULT_NOXWS_PASSWORD, backup=None):
        if backup == None:
            backup = StdStreamPromptedLogin()
        PersistentLogin.__init__(self, user, password, backup)

class FailureExceptionLogin(NOXWSLoginMgr):
    """Fail any attempt at login with an exception.

    This is intended for use in situations like self-tests where a failure
    to login using default credentials is considered an error that should
    result in test failure."""
    def get_credentiails(self):
        raise NOXWSLoginError("Could not login to web service.")

class NOXWSResponse:
    """Wrapper for httlib.HTTPResponse with extra methods for handling body."""
    def __init__(self, httplib_response):
        self._response = httplib_response
        self.status = httplib_response.status
        self.reason = httplib_response.reason
        self.version = httplib_response.version
        self._headers = {}
        self._body = None
        for (k,v) in httplib_response.getheaders():
            l = self._headers.get(k, [])
            l.append(v)
            self._headers[k] = l

    def read(self, amt=None):
        if amt == None:
            return self._response.read()
        else:
            return self._response.read(amt)

    def getHeader(self, name, default=None):
        k = name.lower()
        return self._headers.get(k, default)

    def getHeaders(self):
        return self._headers

    def getContentType(self):
        # For now we assume the last content-type header has the correct
        # content type if there is more than one.
        return self.getHeader("Content-Type", [""])[-1]

    def getBody(self):
        if self._body == None:
            self._body = self._response.read()
        return self._body

    def getExpectedBody(self, expected_status, expected_content_type):
        """Get body of response with required status and content type.

        Raises exceptions if expectations are violated."""
        if self.status != expected_status:
            raise NOXWSStatusCodeError("Return status (%d) != expected (%d)" % (self.status, expected_status), self)
        elif self.getContentType() != expected_content_type:
            raise NOXWSMsgTypeError("Content-Type (%s) != expected (%s)." % (self.getContentType(), expected_content_type), self)
        return self.getBody()

    def okJsonBody(self):
        return simplejson.loads(self.getExpectedBody(httplib.OK, "application/json"))


class NOXWSClient:
    def __init__(self, host, port=DEFAULT_NOXWS_PORT, https=False, loginmgr=None):
        if loginmgr == None:
            loginmgr = DefaultLogin()
        self.loginmgr = loginmgr
        if https:
            self.httpcon = httplib.HTTPSConnection(host, port)
        else:
            self.httpcon = httplib.HTTPConnection(host, port)
        self.cookie = None
        self._closed = True

    def _login(self):
        self.loginmgr.start_auth()
        headers = {}
        headers["Content-Type"] = "application/x-www-form-urlencoded"
        if self.cookie != None:
            headers["Cookie"] = self.cookie
        headers["User-Agent"] = "Python NOXWSClient"
        retry = True
        while retry:
            user, password = self.loginmgr.get_credentials()
            body = urllib.urlencode({ "username": user,
                                      "password": password })
            log.debug("Posting login, username=%s password=%s", user, password)
            self.httpcon.request("POST", "/ws.v1/login", body, headers)
            response = self.httpcon.getresponse()
            self.cookie = response.getheader("Set-Cookie", self.cookie)
            if response.status == httplib.OK:
                retry = False
            self.httpcon.close()
        self.loginmgr.auth_complete()

    def request(self, method, url, headers=None, body=""):
        if not self._closed:
            self.httpcon.close()
        self._closed = False
        if headers == None:
            headers = {}
        headers["User-Agent"] = "Python NOXWSClient"
        while True:
            if self.cookie != None:
                headers["Cookie"] = self.cookie
            log.debug("Making request: method="+ repr(method) + ", url=" + repr(url) + ", headers=" + repr(headers) + ", body=" + body)
            self.httpcon.request(method, url, body, headers)
            response = self.httpcon.getresponse()
            log.debug("Received response: status=" + repr(response.status) + ", headers=" + repr(response.getheaders()))
            self.cookie = response.getheader("Set-Cookie", self.cookie)
            log.debug("Checking response status %d == %d (UNAUTHORIZED)", response.status, httplib.UNAUTHORIZED)
            if response.status == httplib.UNAUTHORIZED:
                log.debug("Got unauthorized response code")
                self.httpcon.close()
                self._login()
                continue
            break
        return NOXWSResponse(response)

    def head(self, url, headers=None):
        return self.request("HEAD", url, headers)

    def get(self, url, headers=None):
        return self.request("GET", url, headers)

    def put(self, url, headers, body):
        return self.request("PUT", url, headers, body)

    def putJson(self, json, url, headers=None):
        if headers == None:
          headers = {}
        headers["content-type"] = "application/json"
        return self.put(url, headers, json)

    def putAsJson(self, object, url, headers=None):
        json = simplejson.dumps(object)
        return self.putJson(json, url, headers)

    def post(self, url, headers, body):
        return self.request("POST", url, headers, body)

    def postJson(self, json, url, headers=None):
        if headers == None:
          headers = {}
        headers["content-type"] = "application/json"
        return self.post(url, headers, json)

    def postAsJson(self, object, url, headers=None):
        json = simplejson.dumps(object)
        return self.postJson(json, url, headers)

    def delete(self, url, headers=None):
        return self.request("DELETE", url, headers)

class CmdLineParser:
    def __init__(self):
        self._parser_sm = {
            "START" : {
                "\\" : ("back_quote", None),
                "\"" : ("double_quote", None),
                "\t" : ("whitespace", self._save_arg),
                " " : ("whitespace", self._save_arg),
                "DEFAULT" : ("START", self._append_char)
                },
            "whitespace" : {
                " " : ("whitespace", None),
                "\t" : ("whitespace", None),
                "\\" : ("back_quote", None),
                "\"" : ("double_quote", None),
                "DEFAULT" : ("START", self._append_char)
                },
            "back_quote" : {
                # TBD: consider supporting C-style backslash escapes
                "DEFAULT" : ("START", self._append_char)
                },
            "double_quote" : {
                "\"" : ( "START", None),
                "\\" : ( "double_quote_back_quote", None),
                "DEFAULT" : ("double_quote", self._append_char)
                },
            "double_quote_back_quote" : {
                "DEFAULT" : ("double_quote", self._append_char)
                }
            }

    def _save_arg(self):
        self.args.append(self.arg)
        self.arg = ""

    def _append_char(self):
        self.arg = self.arg + self.c

    def _process_char(self, c):
        self.c = c
        try:
            self.state, method = self._parser_sm[self.state][self.c]
        except KeyError, e:
            self.state, method = self._parser_sm[self.state]["DEFAULT"]
        if method != None:
            method()

    def _parse_init(self):
        self.state = "START"
        self.arg = ""
        self.args = []

    def parse_arg(self, line, begidx=0, endidx=None):
        """Parse the next arg in line, stopping at end of argument."""
        if endidx == None:
            endidx = len(line)
        self._parse_init()
        for i in range(begidx, endidx):
            self._process_char(line[i])
            if len(self.args) > 0:
                return self.args[0], i
        return self.arg, len(line)

    def parse(self, line):
        """Parse all remaining args in line."""
        self._parse_init()
        for c in line:
            self._process_char(c)
        if self.arg != "":
            self.args.append(self.arg)
        return self.args


class NOXWSClientCmd(Cmd):
    # TBD: - Add completion for command arguments
    # TBD: - Add help for web services paths

    def __init__(self, noxwsclient):
        Cmd.__init__(self)
        self.base_uri = "/"
        self._set_prompt()
        self._wsc = noxwsclient
        self._msgbodies = {}
        self._msgbody_load([])
        self._parser = CmdLineParser()
        self.intro = """\
NOX Web Service Client

Use the command "help" to see all available commands.  You can obtain
additional information on specific commands with "help <command name>".

"""

    def _set_prompt(self):
        self.prompt = "ws client (%s): " % self.base_uri

    def _normurl(self, pathstr, query_param=None, quote=True):
        pc = pathstr.split("/")
        if pc[0] != '':
            pc = self.base_uri.split("/") + pc  # Add base for rel path
        nc = []
        for c in pc:
            if c == '':
                continue
            elif c == '.':
                continue
            elif c == '..':
                nc = nc[:-1]
            else:
                if quote:
                    nc.append(urllib.quote(c, ""))
                else:
                    nc.append(c)
        qpstr = ""
        if query_param != None:
            qpstr = "?"
            sep = ""
            for (k,v) in query_param.items():
                if quote:
                    q = urllib.quote(k, "")
                    q = urllib.quote(v, "")
                qpstr += "%s%s=%s" % (sep, k, v)
                sep="&"
        return "/" + "/".join(nc) + qpstr

    def _splitargs(self, line):
        return self._parser.parse(line)

    def _display_body(self, type, body):
        if type == "application/json":
            print body, "\n"
            print "\n"
        elif type == "text/html":
            # TBD: - display html text better (pass through w3m as filter?)
            print body, "\n"
        elif type.startswith("text/"):
            print body, "\n"
        else:
            print "Body with unknown content type:", type, "\n"

    def _handle_response(self, response):
        if response == None:
            return
        print "%s\n%d %s\n" % ("-" * 78, response.status, response.reason)
        for h,l in response.getHeaders().items():
            for i in l:
                print "%s: %s" % (h,i)
        print "-" * 78
        contentType = response.getContentType()
        body = response.getBody()
        try:
            if contentType == "application/json":
                # Reformat json so easy to work with.
                body = simplejson.dumps(simplejson.loads(body), indent=2)
            if response.status == httplib.OK:
                for i in range (4, 0, -1):
                    b = self._msgbodies.get("^" * (i-1), None)
                    if b != None:
                        self._msgbodies["^" * i] = b
                self._msgbodies["^"] = (contentType, body)
            self._display_body(contentType, body)
        except:
            print "Body did not contain properly formatted JSON data: '%s'"\
                    %body

    def _uri_path(self, args):
        path = "."
        query_param = None
        if len(args) > 0:
            path = args[0]
        return self._normurl(path)

    def _uri_with_query_param(self, args):
        path = "."
        query_param = None
        if len(args) > 0:
            path = args[0]
        if len(args) > 1:
            query_param = {}
            for i in range(1, len(args), 2):
                n = args[i]
                if i+1 >= len(args):
                    v = ""
                else:
                    v = args[i+1]
                query_param[n] = v
        return self._normurl(path, query_param)

    def help_cd(self):
        print """
usage: cd <path>

Change the base URI path as specified by the relative or absolute path
argument.
"""

    def do_cd(self, line):
        args = self._splitargs(line)
        if len(args) != 1:
            print "\nIncorrect number of arguments\n"
            self.help_cd()
            return
        self.base_uri = self._normurl(args[0], quote=False)
        self._set_prompt()
        return False

    def help_head(self):
        print """
usage: head <path> [<param_name> <param_value>]...

Perform a head request for the specified path, which can be absolute or
relative to the current base path.  Query parameters can be optionally
specified.
"""

    def do_head(self, line):
        args = self._splitargs(line)
        url = self._uri_with_query_param(args)
        try:
            response = self._wsc.head(url)
        except socket.error, e:
            print "\nRequest failed: ", str(e)
            return None
        self._handle_response(response)
        return False

    def help_get(self):
        print """
usage: get <path> [<param_name> <param_value>]...

Perform a get request for the specified path, which can be absolute or
relative to the current base path.  Query parameters can be optionally
specified.
"""

    def do_get(self, line):
        args = self._splitargs(line)
        url = self._uri_with_query_param(args)
        try:
            response = self._wsc.get(url)
        except socket.error, e:
            print "\nRequest failed: ", str(e)
            return None
        self._handle_response(response)
        return False

    def help_put(self):
        print """
usage: put <msg body name> <path>

Perform a put request with the specified message body.
"""

    def do_put(self, line):
        args = self._splitargs(line)
        if len(args) != 2:
            print "\nIncorrect number of arguments\n"
            self.help_put()
            return
        n = args[0]
        url = self._uri_path(args[1:])
        t, b = self._msgbodies.get(n, (None, None))
        if t == None:
            print "\nThe message body named %s does not exist\n" % n
            return
        try:
            response = self._wsc.put(url, { "Content-Type" : t }, b)
        except socket.error, e:
            print "\nRequest failed: ", str(e)
            return None
        self._handle_response(response)
        return False

    def help_post(self):
        print """
usage: post <msg body name> <path>

Perform a post request with the specified message body.
"""

    def do_post(self, line):
        args = self._splitargs(line)
        if len(args) != 2:
            print "\nIncorrect number of arguments\n"
            self.help_post()
            return
        n = args[0]
        url = self._uri_path(args[1:])
        t, b = self._msgbodies.get(n, (None, None))
        if t == None:
            print "\nThe message body named %s does not exist\n" % n
            return
        try:
            response = self._wsc.post(url, { "Content-Type" : t }, b)
        except socket.error, e:
            print "\nRequest failed: ", str(e)
            return None
        self._handle_response(response)
        return False

    def help_delete(self):
        print """
usage: delete <path>

Perform a delete request for the specified path, which can be absolute or
relative to the current base path.
"""

    def do_delete(self, line):
        args = self._splitargs(line)
        if len(args) > 1:
            print "\nToo many arguments to command."
            self.help_delete()
        else:
            url = self._uri_path(args)
            try:
                response = self._wsc.delete(url)
            except socket.error, e:
                print "\nRequest failed: ", str(e)
                return None
            self._handle_response(response)
        return False

    def help_msgbody(self):
        print """
usage: msgbody list
       msgbody show [<name>]
       msgbody edit [<name>] [<content-type>]
       msgbody copy [<src name>] <dst name>
       msgbody delete <name>
       msgbody save [<file>]
       msgbody load [<file>]

The msgbody commands allow manipulating messsage bodies received from
the web service or to be submitted to the web service in PUT and POST
requests.

The <name> parameters can be any string except for the special values
"^", "^^", "^^^", "^^^^", and "^^^^^".  These values always refer to
the message bodies from the last first through fifth successful
requests made.  If these are used in as the destination of a copy, or
in the edit and delete commands, the bodies will be modified.  Use
with care.

If the load command is issued multiple times the contents of all the
loaded files merged.  If there are overlapping names in the files, the
version most recently loaded will be used with the exception of the special
values references previous message bodies.  These will only be set if they
are not yet set.  The default file used if none is specifed is:

        %s

This file is loaded automatically at startup and stored automatically
at exit.
""" % DEFAULT_MSGBODY_FILE

    def _msgbody_list(self, args):
        keys = sorted(self._msgbodies.keys())
        for k in keys:
            print "%-20s %s" % (k, self._msgbodies[k][0])

    def _msgbody_show(self, args):
        if len(args) == 0:
            n = "^"
        elif len(args) == 1:
            n = args[0]
        else:
            print "\nToo many arguments to show subcommand."
            self.help_msgbody()
            return
        t, b = self._msgbodies.get(n, (None, None))
        if t == None:
            print "\nThe message body named %s does not exist.\n" % n
            return
        self._display_body(t, b)

    def _msgbody_edit(self, args):
        t = None
        if len(args) == 0:
            n = "^"
        elif len(args) == 1:
            n = args[0]
        elif len(args) == 2:
            n = args[0]
            t = args[1]
        else:
            print "\nToo many arguments for edit subcommand."
            self.help_msgbody()
            return
        (ot, b) = self._msgbodies.get(n, (None, None))
        if b == None:
            print "\nThe message body named %s does not exist." % n
            if t != None:
                print "Creating an empty message body of content-type %s.\n" % t
                b = ""
            else:
                print "If you specify the content-type after the name you can create a new"
                print "item with the specified content-type.\n"
                return
        if t == None:
            t = ot
        h, fn = mkstemp();
        h = open(fn, "w")
        h.write(b)
        h.close()
        editor = environ.get("EDITOR")
        if editor == None:
            print "\nThe EDITOR environment variable is not set, can't edit.\n"
            return
        subprocess.call([ editor, fn ])
        h = open(fn, "r")
        b = h.read()
        h.close()
        self._msgbodies[n] = (t, b)

    def _msgbody_copy(self, args):
        if len(args) == 0:
            print "Not enough arguments for copy subcommand.\n"
            self.help_msgbody()
            return
        elif len(args) == 1:
            src = "^"
            dst = args[0]
        elif len(args) == 2:
            src = args[0]
            dst = args[1]
        else:
            print "Too many arguments for copy subcommand.\n"
            self.help_msgbody()
            return
        b = self._msgbodies.get(src, None)
        if b == None:
            print "The message body named %s does not exist.\n" % src
            return
        self._msgbodies[dst] = b

    def _msgbody_delete(self, args):
        if len(args) != 1:
            print "Incorrect number of arguments for delete subcomand.\n"
            self.help_msgbody()
            return
        if self._msgbodies.has_key(args[0]):
            del(self._msgbodies[args[0]])

    def _get_filename(self, args):
        if len(args) == 0:
            fn = DEFAULT_MSGBODY_FILE
        elif len(args) == 1:
            fn = args[0]
        else:
            print "\nToo many arguments.\n"
            self.help_msgbody()
            return None
        if fn.startswith("~/"):
            fn = path.abspath(path.join(environ["HOME"], fn[2:]))
        return fn

    def _msgbody_save(self, args):
        fn = self._get_filename(args)
        if fn == None:
            return
        f = None
        try:
            f = open(fn, "w")
            f.write(repr(self._msgbodies))
        except IOError, e:
            print "\nCould not save to file:", str(e), "\n"
        if f != None:
            f.close()

    def _msgbody_load(self, args):
        fn = self._get_filename(args)
        if fn == None:
            return
        try:
            f = open(fn, "r")
        except IOError, e:
            print "\nCould not load from file:", fn, "\n"
            return
        s = f.read()
        f.close()
        d = eval(s)
        for k in d:
            if k not in [ "^", "^^", "^^^", "^^^^", "^^^^^" ]:
                self._msgbodies[k] = d[k]
            elif not self._msgbodies.has_key(k):
                self._msgbodies[k] = d[k]

    def do_msgbody(self, line):
        args = self._splitargs(line)
        if len(args) == 0:
            print "\nNo subcommand was specified."
            self.help_msgbody()
            return
        elif args[0] == "list":
            self._msgbody_list(args[1:])
        elif args[0] == "show":
            self._msgbody_show(args[1:])
        elif args[0] == "edit":
            self._msgbody_edit(args[1:])
        elif args[0] == "copy":
            self._msgbody_copy(args[1:])
        elif args[0] == "delete":
            self._msgbody_delete(args[1:])
        elif args[0] == "save":
            self._msgbody_save(args[1:])
        elif args[0] == "load":
            self._msgbody_load(args[1:]);
        else:
            print "\nUnknown subcommand: %s" % args[0]
            self.help_msgbody()
        return False

    def help_quit(self):
        print """
usage: quit

Quit the client.
"""

    def do_quit(self, line):
        return True

    def help_help(self):
        print """
usage: help [<command>]

Get a list of all commands or help on a specific command.
"""

    def preloop(self):
        try:
            readline.read_history_file(HISTORY_FILE)
        except IOError, e:
            # Assume can't read and forget about it
            pass

    def postloop(self):
        self._msgbody_save([])
        try:
            readline.write_history_file(HISTORY_FILE)
        except IOError, e:
            # Assume can't write and forget about it
            pass
        print "\n"


if __name__ == "__main__":
    parser = OptionParser(usage="usage: %prog [options] <dest>")
    parser.add_option("-u", "--user", dest="user",
                      type="string", default=DEFAULT_NOXWS_USER,
                      help="Username to provide for login (default=%default).")
    parser.add_option("-p", "--password", dest="password",
                      type="string", default=DEFAULT_NOXWS_USER,
                      help="Password to provide for login (default=%default).")
    # TBD: - Change default to use SSL
    parser.add_option("--ssl", dest="ssl",
                      action="store_true", default=False,
                      help="Use SSL.")
    parser.add_option("--no-ssl", dest="ssl",
                      action="store_false", default=False,
                      help="Do not use SSL. (default)")
    parser.add_option("-P", "--port", dest="port",
                      type="int", default=None,
                      help="Destination port number to connect to.")

    (options, args) = parser.parse_args()
    if options.port == None:
        if options.ssl:
            options.port = 443
        else:
            options.port = DEFAULT_NOXWS_PORT
    if len(args) != 1:
        parser.print_help()
        sys.exit(1)
    options.host = args[0]

    loginmgr = PersistentLogin(options.user, options.password)
    wsc = NOXWSClient(options.host, options.port, options.ssl, loginmgr)
    cmd = NOXWSClientCmd(wsc)
    try:
        cmd.cmdloop()
    except KeyboardInterrupt, e:
        cmd.postloop()
