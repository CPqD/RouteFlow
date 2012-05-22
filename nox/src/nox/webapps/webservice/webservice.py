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
"""Web services component module

==== Overview ====

This module implements the component providing the web services
interface for NOX applications.  Other NOX applications use this
module to register to handle specific web services requests and
generate appropriate responses.


==== Background ====

The NOX web services are intended to implement a RESTful web service
API, roughly following the Resources Oriented Architecture described
in _RESTful Web Services_ by Leonard Richardson & Sam Ruby.  See that
book for a detailed explanation.  Briefly however, the key ideas are:

   1) Addressability: Every interesting piece of data and algorithm
      exposed by the application can be addressed as a separate resource.
      URIs are the method of addressing resources in HTTP, so effectively
      this means each has its own URI.  The message body content of a
      request is NOT used to determine the data to be affected.  Ideally
      the URIs are descriptive and human-readable.

   2) Statelessness: Every request happens in isolation.  The server
      does not base future actions on past client behavior except
      as that behavior affected the resources exposed by the server.  In
      practice, this means that the application should not uses session
      variables in dealing with requests.

   3) Connectedness: Where possible, the application should return links
      to other related resources in the representations it serves to the
      client, allowing the client to navigate the available resources
      without having to understand all possible URIs.

   4) Uniform Interface: The web service should have a uniform resource
      addressing and representations.  Uniform addressing is accomplished
      by using the standard HTTP request methods in their "natural"
      interpretation along with the resource-specific URIs.  The request
      methods typically used are:

             GET: Retrieve a respresentation of a resource.
            POST: Create a new resource that doesn't have a known
                  URI before it is created.
             PUT: Modify an existing resource or create a new resource
                  at a URI known before the resource is created.
          DELETE: Delete an existing resource.

      Uniform representations is accomplished by representing resources
      using a standard encoding.  In many web services, this is XML.
      For NOX, because the primary client for web services is expected
      to be web browsers, Javascript Object Notation (JSON) is used.
      Error responses are also made uniform by using the standard HTTP
      response codes.


==== Registering for Requests ====

Applications implement a web services interface by registering to
respond to specific requests with the information appropriate to that
application.  To register for a URI, an application must follow these
steps:

    1) Resolve the webservices component

       ws = component.resolve(str(nox.webapps.webserver.webservice))

    2) Get an object representing a specific version of a web
       services interface.  This is to allow for future incompatible
       changes to the interface should they be required.

       v1 = ws.get_version("1")

    3) Call the register_request method on that object.

       v1.register_request(handler, request_method, path_components, doc)

The parameters given to register_request describe a set of requests
(HTTP request method + URI paths) for which the given handler callback
will be called and provide some documentation about the purpose of the
request.  With the exception of the path_components parameter they are
straightforward:

    handler: The callback function to be called to handle the request.
             It will be called with two parameters, the HTTP web server
             request object and a dictionary of data extracted from the
             URI as described by the path_components parameter.
    request_method: The HTTP request method (for example, GET, PUT,
             POST, etc.)
    doc: A string explaining the purpose of the request.

The path_components parameter describes the URIs to be handled.  It
contains a sequence (list, tuple, etc.) of WSPathComponent subclass
instances.  The simplest such subclass is WSPathStaticString.  This
code:

    path_components = ( WSPathStaticString("abc"),
                        WSPathStaticString("xyz") )
    v1.register_request(h, "GET", path_components, "Some doc")

Will register the "h" callback function to be called for the request:

    GET /ws.v1/abc/xyz

While often required, matching on static strings could have been
handled by a much simpler mechanism.  The reason for the current
implementation is to support extracting and verifying dynamic data
from URIs.  As an example, consider a request to delete a user login
account.  The form of the request could be:

    DELETE /ws.v1/user/<user name>

where <user name> represents a valid existing login name.  If there is
a "userdb" object that provides the method "isValidLogin" to determine
if a login is valid, a WSPathComponent subclass to extract and verify
the login name can be implemented as:

    class WSPathUserLogin(WSPathComponent):
        def __init__(self, userdb):
            webservices.WSPathComponent.__init__(self)
            self.userdb = userdb   # Keep ref to userdb for later use

        def __str__(self):
            return "<login name>"

        def extract(self, path_component_str, data):
            if self.userdb.isValidLogin(path_component_str):
                return WSPathExtractResult(value=path_component_str)
            else:
                e = "'%s' is not a valid login name." % path_component_str
                return WSPathExtractResult(error=e)

To handle the URI we would register it as:

    path_components = ( WSPathStaticString("user"),
                        WSPathUserLogin(userdb) )
    v1.register_request(d, "DELETE", path_components, "Delete user login.")

The __str__ method of WSPathUserLogin is particularly important
because it is used in several ways:

    1) The request handler constructs a tree of request paths for
       efficient URI path matching.  It assumes that two WSPathComponent
       subclass instances that return the same value for __str__() do the
       same thing and will keep only one of the multiple instances that
       might have been registered for different paths that share the same
       prefix.  Therefore it should be different if the class behaves
       differently depending on how it is initialized.  See WSPathStaticString
       for an example.

    2) The request handler also auto-generates informative error messages
       and a documentation page describing valid request templates.
       Conceptually, it constructs the user displayed path by
       evaluating the python expression:

            "/" + "/".join([str(pc) for pc in path_components])

       For this reason, by convention subclasses that extract dynamic data
       should return a string enclosed in angle brackets ('<', and '>')
       as done in the user login example above.

    3) The dictionary of data extracted from the URI is keyed by this
       value as well.  This dictionary is passed to both extract() method
       calls on subsequent path components and to the final callback
       function.  For this reason, the value returned should be predictable,
       typically a constant string or a string that varies in a predictable
       way.


==== Handling Requests ====

Once the request handler has determined the correct callback function
to handle a request it calls it with the web server request object and
the extracted data dictionary.  The callback function must generate
the response data to be sent to the client.  One method of doing so is
to return that data immediately.  A simple example is:

    def hello_user_handler(request, data):
        request.setHeader("Content-Type", "text/plain")
        return "Hello %s!\n" % (data["<login name>"],)

If this were registered as follows (using the previously defined
WSPathUserLogin class):

    pc = ( WSPathStaticString("hello"),
           WSPathUserLogin(userdb) )
    v1.register_request(hello_user_handler, "GET", pc, "Say hello to <user>.")

Then the request:

    GET /ws.v1/hello/john_smith

would result in a text document containing the single line:

    Hello john_smith!

Assuming of course, that john_smith was a valid login name.  If not,
the request handler would return a not found error with information
about why the request could not be satisified without calling the
handler callback at all.

If the data to be returned is generated from a deferred or other
asynchronous mechanism, this is not possible.  In that case, the
handler should return NOT_DONE_YET and output the content using one or
more calls of request.write() method followed by a call to the
request.finish() method.  For example:

    class DeferredExample:
        def __init__(self, request):
            self.request = request

        def ok(self, res):
            self.request.write("The result was successful.")
            self.request.finish()

        def do_deferred(self):
            d = someDeferredOp()
            d.addCallback(self.ok)

    def deferred_handler(request, data):
        DeferredExample(request).do_deferred()
        return NOT_DONE_YET

This module provides several functions to facilitate handling
requests.  First, there are a set of general functions to generate
uniform HTTP error responses of the same names.  These are:

    badRequest, conflictError, and internalError

There are also a set of special purpose functions for other error
codes that are typically used by the request handler itself and not
the handler callbacks, but may be needed by the callbacks in special
circumstances.  These include:

    notFound, methodNotAllowed, unauthorized, and forbidden

The last of these (forbidden) is related to authorization checking.
This *is* the responsibility of the handler callback.  The
authorization system is based on sets of capabilities identified by
strings registered by the application.  The handler callback should
check the client is logged in as a user with the required capabilities
to perform the request operation.  Rather than doing its own tests and
calling forbidden with its own error message, the handler callback
will typically check permissions using the authorization_failed
function.  A simple example is:

    def simple_access_check_handler(request, data):
        if authorization_failed(request, [set("capability1", "capability2")]):
            return NOT_DONE_YET
        request.setHeader("Content-Type", "text/plain")
        return "Congratulations, you are authorized to see this page!"

Finally, the uniform resource representation for NOX applications is
Javascript Object Notation (JSON).  By default, the request handler
sets the content-type of the response to 'application/json' before
calling the handler callback.  The callback shouldn't generally need
to overide this but can if required.  To generate JSON output, it is
recommended that the callback handler use the simplejson library.

The requests with the PUT and POST methods often contain a message
body that will also be JSON encoded.  To create a corresponding python
object representation (of nested dictionaries, lists, and literal
values) this module provides the json_parse_message_body function.
This function will ensure the message body has the correct content
type and that it parses as a valid JSON representation, outputting
uniform error messages if these tests fail.  It returns the parsed
object if it succeeds or None if it fails.  In the latter case it has
already generated the error response for the client so the handler
callback should just return NOT_DONE_YET, with something like:

    # ... setup at the beginning of the handler callback
    content = json_parse_message_body(request)
    if content == None:
        return NOT_DONE_YET
    # further validate rep and continue processing here...


==== Testing an Application Web Services Interface ====

If NOX is started with just the web services component being developed
and the webservice_testui component, a web page can be accessed at
http://127.0.0.1:8888/ that provides a way to specify the request
method, URI, and message body of test requests, submit the request,
and view the result returned by the server.  This is the easiest way
to test the implementation.
"""

from nox.coreapps.pyrt.pycomponent import *
from nox.lib.core import *
from nox.lib import config

from nox.webapps.webserver import webserver
from nox.webapps.webserver import webauth

import re
import textwrap
import simplejson
import logging
import traceback
from copy import copy

from twisted.web import server
from twisted.python.failure import Failure

lg = logging.getLogger('webservice')

NOT_DONE_YET = server.NOT_DONE_YET

### Response functions:
#
# The following functions can be used to generate various error responses.
# These should only ever be used for the web-services interface, not the
# user-facing web interface.

def forbidden(request, errmsg, otherInfo={}):
    """Return an error code indicating client is forbidden from accessing."""
    request.setResponseCode(403)
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = errmsg
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def badRequest(request, errmsg, otherInfo={}):
    """Return an error indicating a problem in data from the client."""
    request.setResponseCode(400, "Bad request")
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = "The server did not understand the request."
    d["error"] = errmsg
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def conflictError(request, errmsg, otherURI=None, otherInfo={}):
    """Return an error indicating something conflicts with the request."""
    if otherURI != None:
        request.setResponseCode(409, "Conflicts with another resource")
        request.setHeader("Location", otherURI.encode("utf-8"))
    else:
        request.setResponseCode(409, "Internal server conflict")
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = "Request failed due to simultaneous access."
    d["error"] = errmsg
    d["otherURI"] = otherURI
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def internalError(request, errmsg, otherInfo={}):
    """Return an error code indicating an error in the server."""
    request.setResponseCode(500)
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = "The server failed while attempting to perform request."
    d["error"] = errmsg
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def notFound(request, errmsg, otherInfo={}):
    """Return an error indicating a resource could not be found."""
    request.setResponseCode(404, "Resource not found")
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = "The server does not have data for the request."
    d["error"] = errmsg
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def methodNotAllowed(request, errmsg, valid_methods, otherInfo={}):
    """Return an error indicating this request method is not allowed."""
    request.setResponseCode(405, "Method not allowed")
    method_txt = ", ".join(valid_methods)
    request.setHeader("Allow", method_txt)
    request.setHeader("Content-Type", "application/json")
    d = copy(otherInfo)
    d["displayError"] = "The server can not perform this operation."
    d["error"] = errmsg
    d["validMethods"] = valid_methods
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET

def unauthorized(request, errmsg="", otherInfo={}):
    """Return an error indicating a client was not authorized."""
    request.setResponseCode(401, "Unauthorized")
    request.setHeader("Content-Type", "application/json")
    if errmsg != "":
        errmsg = ": " + errmsg
    d = copy(otherInfo)
    d["displayError"] = "Unauthorized%s\n\n" % (errmsg, )
    d["error"] = errmsg
    d["loginInstructions"] = "You must login using 'POST /ws.v1/login' and pass the resulting cookie with\neach request."
    request.write(simplejson.dumps(d))
    request.finish()
    return server.NOT_DONE_YET


### Authorization
#
def authorization_failed(request, capabilities_sets):
    """Verify authorization and output error message if not allowed.

    Checks for authorization (using `webauth.requestIsAllowed').  If
    authorization succeeds, returns False.  If authorization fails,
    returns True after generating an error response, in which case the
    caller should return webservice.NOT_DONE_YET to inform the web sever
    to send the error response.

    The capabilities_sets argument is a list of one or more sets of
    capabilities that the user must have for authorization to succeed.
    The user must hold all of the capabilities in at least one of the
    sets in the list for authorization to succeed."""
    for s in capabilities_sets:
        if webauth.requestIsAllowed(request, s):
            return False

    session = webserver.get_current_session(request)
    e = """This request requires one of the following capabilities sets:

    %s

You are logged in as user %s which does not have one or more of these
capabilities sets.""" % ("\n".join([", ".join(s) for s in capabilities_sets]), session.user.username)
    forbidden(request, e)
    return True

### Message Body handling
#
def json_parse_message_body(request):
    content = request.content.read()
    content_type = request.getHeader("content-type")
    if content_type == None or content_type.find("application/json") == -1:
        e = [ "The message body must have Content-Type application/json\n",
              "instead of %s. " % content_type ]
        if content_type == "application/x-www-form-urlencoded":
            e.append("The web\nserver decoded the message body as:\n\n")
            e.append(str(request.args))
        else:
            e.append("The message body was:\n\n")
            e.append(content)
        lg.error("".join(e))
        return None
    if len(content) == 0:
        lg.error("Message body was empty.  It should be valid JSON encoded data for this request.")
        return None
    try:
        data = simplejson.loads(content)
    except:
        lg.error("Message body is not valid json data. It was:\n\n%s" % (content,))
        return None
    return data


class WhitespaceNormalizer:
    def __init__(self):
        self._re = re.compile("\s+")

    def normalize_whitespace(self, s):
        return self._re.sub(" ", s).strip()

class WSPathTreeNode:
    _wsn = WhitespaceNormalizer()

    def __init__(self, parent, path_component):
        self.path_component = path_component
        self._handlers = {}
        self._parent = parent
        self._children = []
        self._tw = textwrap.TextWrapper()
        self._tw.width = 78
        self._tw.initial_indent = " " * 4
        self._tw.subsequent_indent = self._tw.initial_indent

    def parent(self):
        return self._parent()

    def _matching_child(self, path_component):
        for c in self._children:
            if str(c.path_component) == str(path_component):
                return c
        return None

    def has_child(self, path_component):
        return self._matching_child(path_component) != None

    def add_child(self, path_component):
        c = self._matching_child(path_component)
        if c == None:
            c = WSPathTreeNode(self, path_component)
            self._children.append(c)
        return c

    def path_str(self):
        if self._parent == None:
            return ""
        return self._parent.path_str() + "/" + str(self.path_component)

    def set_handler(self, request_method, handler, doc):
        if self._handlers.has_key(request_method):
            raise KeyError("%s %s is already handled by '%s'" % (request_method, self.path_str(), repr(self._handlers[request_method][0])))
        d = self._wsn.normalize_whitespace(doc)
        d = self._tw.fill(d)
        self._handlers[request_method] = (handler, d)

    def interface_doc(self, base_path):
        msg = []
        p = base_path + self.path_str()
        for k in self._handlers:
            msg.extend((k, " ", p, "\n"))
            doc = self._handlers[k][1]
            if doc != None:
                msg.extend((doc, "\n\n"))
        for c in self._children:
            msg.append(c.interface_doc(base_path))
        return "".join(msg)

    def handle(self, t):
        s = t.next_path_string()
        if s != None:
            r = None
            if len(self._children) == 0:
                t.request_uri_too_long()
            for c in self._children:
                r = c.path_component.extract(s, t.data)
                if r.error == None:
                    t.data[str(c.path_component)] = r.value
                    t.failed_paths = []
                    r = c.handle(t)
                    break
                else:
                    t.failed_paths.append((c.path_str(), r.error))
            if len(t.failed_paths) > 0:
                return t.invalid_request()
            return r
        else:
            try:
                h, d = self._handlers[t.request_method()]
            except KeyError, e:
                return t.unsupported_method(self._handlers.keys())
            return t.call_handler(h)

class WSPathTraversal:

    def __init__(self, request):
        self._request = request
        self._pathiter = iter(request.postpath)
        self.data = {}
        self.failed_paths = []

    def request_method(self):
        return self._request.method

    def next_path_string(self):
        try:
            return self._pathiter.next()
        except StopIteration, e:
            return None

    def call_handler(self, handler):
        self._request.setHeader("content-type", "application/json")
        try:
          return handler(self._request, self.data)
        except Exception, e:
          lg.error("caught unhandled exception with path '%s' : %s" % \
              (str(self._request.postpath), Failure()))
          internalError(self._request,"Unhandled server error")
          return NOT_DONE_YET


    def _error_wrapper(self, l):
        msg = [ ]
        msg.append("You submitted the following request.\n\n")
        msg.append("    %s %s\n\n" % (self._request.method, self._request.path))
        msg.append("This request is not valid. ")
        msg.extend(l)
        msg.append("\n\nYou can get a list of all valid requests with the ")
        msg.append("following request.\n\n    ")
        msg.append("GET /" + "/".join(self._request.prepath) + "/doc");
        return "".join(msg)

    def request_uri_too_long(self):
        e = [ "The request URI path extended beyond all available URIs." ]
        return notFound(self._request, self._error_wrapper(e))

    def unsupported_method(self, valid_methods):
        if len(valid_methods) > 0:
            e = [ "This URI only supports the following methods.\n\n    " ]
            e.append(", ".join(valid_methods))
        else:
            e = [ "There are no supported request methods\non this URI. " ]
            e.append("It is only used as part of longer URI paths.")
        return methodNotAllowed(self._request, self._error_wrapper(e), valid_methods)

    def invalid_request(self):
        e = []
        if len(self.failed_paths) > 0:
            e.append("The following paths were evaluated and failed\n")
            e.append("for the indicated reason.")
            for p, m in self.failed_paths:
                e.append("\n\n    - %s\n      %s" % (p, m))
        return notFound(self._request, self._error_wrapper(e))


### Registering for requests
#
class WSRequestHandler:
    """Class to determine appropriate handler for a web services request."""

    def __init__(self):
        self._path_tree = WSPathTreeNode(None, None)

    def register(self, handler, request_method, path_components, doc=None):
        """Register a web services request handler.

        The parameters are:

            - handler: a function to be called when the specified request
                  method and path component list are matched.  It must
                  have the signature:

                       handler(request, extracted_data)

                  Here the 'request' parameter is a twisted request object
                  to be used to output the result and extracted_data is a
                  dictionary of data extracted by the WSPath subclass
                  instances in the 'path_components' parameter indexed
                  by str(path_component_instance).

            - request_method: the HTTP request method of the request to
                  be handled.

            - path_components: a list of 'WSPathComponent' subclasses
                  describing the path to be handled.

            - doc: a string describing the result of this request."""
        pn = self._path_tree
        for pc in path_components:
            pn = pn.add_child(pc)
        pn.set_handler(request_method.upper(), handler, doc)

    def handle(self, request):
        return self._path_tree.handle(WSPathTraversal(request))

    def interface_doc(self, base_path):
        """Text describing all current valid requests."""
        d = """\
This is a RESTful web interface to NOX network applications.  The applications
running on this NOX instance support the following requests.\n\n"""

        return d + self._path_tree.interface_doc(base_path)

class WSPathExtractResult:
    def __init__(self, value=None, error=None):
        self.value = value
        self.error = error


class WSPathComponent:
    """Base class for WS path component extractors"""

    def __init__(self):
        """Initialize a path component extractor

        Currently this does nothing but that may change in the future.
        Subclasses should call this to be sure."""
        pass

    def __str__(self):
        """Get the string representation of the path component

        This is used in generating information about the available paths
        and conform to the following conventions:

            - If a fixed string is being matched, it should be that string.
            - In all other cases, it should be a description of what is
              being extracted within angle brackets, for example,
              '<existing database table name>'.

        This string is also the key in the dictionary callbacks registered
        with a WSPathParser instance receive to obtain the extracted
        information."""
        err = "The '__str__' method must be implemented by subclasses."
        raise NotImplementedError(err)

    def extract(self, pc, extracted_data):
        """Determine if 'pc' matches this path component type

        Returns a WSPathExtractResult object with value set to the
        extracted value for this path component if the extraction succeeded
        or error set to an error describing why it did not succeed.

        The 'pc' parameter may have the value 'None' if all path components
        have been exhausted during previous WS path parsing. This is
        to allow path component types that are optional at the end
        of a WS.

        The extracted_data parameter contains data extracted
        from earlier path components, which can be used during the
        extraction if needed.  It is a dictionary keyed by the
        str(path_component) for each previous path component."""
        err = "The 'extract' method must be implemented by subclasses."
        raise NotImplementedError(err)

class WSPathStaticString(WSPathComponent):
    """Match a static string in the WS path, possibly case insensitive."""

    def __init__(self, str, case_insensitive=False):
        WSPathComponent.__init__(self)
        self.case_insensitive = case_insensitive
        if case_insensitive:
            self.str = str.lower()
        else:
            self.str = str

    def __str__(self):
        return self.str

    def extract(self, pc, data):
        if pc == None:
            return WSPathExtractResult(error="End of requested URI")

        if self.case_insensitive:
            if pc.lower() == self.str:
                return WSPathExtractResult(value=pc)
        else:
            if pc == self.str:
                return WSPathExtractResult(value=pc)
        return WSPathExtractResult(error="'%s' != '%s'" % (pc, self.str))

class WSPathRegex(WSPathComponent):
    """Match a regex in the WS path.

    This can not be used directly but must be subclassed.  Typically
    the only thing a subclass must override is the '__str__'
    method.

    The value returned from the 'extract' method is the python regular
    expression match object, from subgroups in the expression can be
    examined, etc."""
    def __init__(self, regexp):
        WSPathComponent.__init__(self)
        self.re = re.compile(regexp)

    def extract(self, pc, data):
        if pc == None:
            return WSPathExtractResult(error="End of requested URI")
        m = re.match(pc)
        if m == None:
            return WSPathExtractResult(error="Regexp did not match: %s" % self.re.pattern)
        return WSPathExtractResult(value=m)

class WSPathTrailingSlash(WSPathComponent):
    """Match a null string at a location in the WS path.

    This is typically used at the end of a WS path to require a
    trailing slash."""

    def __init__(self):
        WSPathComponent.__init__(self)

    def __str__(self):
        return "/"

    def extract(self, pc, data):
        if pc == "":
            return WSPathExtractResult(True)
        else:
            return WSPathExtractResult(error="Data following expected trailing slash")

# match any string, and retrieve it by 'name'
# (e.g.,  WSPathArbitraryString('<hostname>') 
class WSPathArbitraryString(WSPathComponent):
  def __init__(self, name):
    WSPathComponent.__init__(self)
    self._name = name
  
  def __str__(self):
    return self._name 
  
  def extract(self, pc, data):
    if pc == None:
      return WSPathExtractResult(error="End of requested URI")
    return WSPathExtractResult(unicode(pc, 'utf-8'))


class WSRes(webauth.AuthResource):
    isLeaf = True
    required_capabilities = set([])

    def _get_interface_doc(self, request, arg):
        request.setHeader("Content-Type", "text/plain")
        return self.mgr.interface_doc("/" + "/".join(request.prepath))

    def _login(self, request, arg):
        username = request.args["username"][0]
        password = request.args["password"][0]
        d = self.webauth.authenticateUser(request, username, password)
        d.addCallback(self._auth_callback, request)
        d.addErrback(self._auth_errback, request)
        return server.NOT_DONE_YET

    def _auth_callback(self, res, request):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "text/plain")
        request.write("Successful Authentication\n\n")
        request.write("You successfully authenticated.  Use the cookie in")
        request.write("this reply in future requests.\n")
        request.finish()

    def _auth_errback(self, failure, request):
        unauthorized(request, "Login failed.")

    def __init__(self, component, version=1):
        self.version = version
        self.loginUri = "/ws.v" + str(self.version) + "/login"
        webauth.AuthResource.__init__(self, component)
        self.mgr = WSRequestHandler()
        self.webauth = component.resolve(webauth.webauth)
        self.register_request(self._get_interface_doc,
                              "GET", ( WSPathStaticString("doc"), ),
                              """Get a summary of requests supported by this
                                 web service interface.""")
        self.register_request(self._login,
                              "POST", ( WSPathStaticString("login"), ),
                              """Login to the webservice.""")

    def register_request(self, handler, request_method, path_components, doc):
        self.mgr.register(handler, request_method, path_components, doc)

    def render(self, request):
        if request.path == self.loginUri:
            # No auth needed to auth... :-)
            return self.mgr.handle(request)
        if not self.webauth.is_initialized(): 
            return internalError(request,"Server Uninitialized")
        if not self.webauth.requestIsAuthenticated(request):
            return unauthorized(request)
        if authorization_failed(request, [self.required_capabilities]):
            return NOT_DONE_YET
        return self.mgr.handle(request)

class webservice(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self._versions = {}
        self.webserver = None


    def get_version(self, version):
        return self._versions[version]

    def install(self):
        self._versions["1"] = WSRes(self, 1)
        self._versions["2"] = WSRes(self, 2)
        self.webserver = self.resolve(str(webserver.webserver))
        for v in self._versions:
            self.webserver.install_resource("/ws.v%s" % v, self._versions[v])

    def getInterface(self):
        return str(webservice)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return webservice(ctxt)

    return Factory()
