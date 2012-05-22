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

from twisted.web import server
from twisted.web.resource import Resource
from twisted.internet.defer import Deferred

from nox.coreapps.pyrt.pycomponent import *
from nox.lib.core import *
from nox.lib.directory import AuthResult
from nox.lib import config


import os
import types
import urllib
import webserver

from webserver import get_current_session

all_immutable_roles = ["Superuser",
                       "Admin",
                       "Demo",
                       "Readonly"]

class UnknownCapabilityError(Exception):
    pass

class InvalidRoleError(Exception):
    pass

class webauth(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.webserver = None
        self.directorymanager = None
        try: 
          from nox.ext.apps.directory.directorymanager import directorymanager
          self.directorymanager = self.resolve(directorymanager)
        except:
          pass
        self.initialized = False

    def setup_immutable_roles(self):
      # Create and register the immutable roles
      for name in all_immutable_roles:
        if name == "Superuser":
            r = SuperuserRole(name)
        elif name == "No Access":
            r = NoAccessRole(name)
        else:
            r = Role(name, immutable=True)
        Roles.register(r)

      self.webserver.authui_initialized = True

    def bootstrap_complete_callback(self, *args):
        self.setup_immutable_roles()
        self.initialized = True
        return CONTINUE

    def is_initialized(self): 
      return self.initialized

    def install(self):
        self.webserver = self.resolve(str(webserver.webserver))
        self.register_for_bootstrap_complete(self.bootstrap_complete_callback)

    def requestIsAuthenticated(self, request):
        session = get_current_session(request)

        if hasattr(session, "roles"):
            return True

        if not self.directorymanager or not self.directorymanager.supports_authentication():
            session.requestIsAllowed = requestIsAllowed
            user = User("assumed-admin", set(("Superuser",)))
            session.user = user
            roles = [ Roles.get(r) for r in user.role_names ]
            session.roles = roles
            session.language = "en"
            return True

        return False

    def authenticateUser(self, request, username, password):
        """Authenticate a user

        Since this is asynchronous, it returns a deferred.  If
        authentication succeeds, the callback is called.  If it fails,
        the errback is called."""
        callerdeferred = Deferred()
        if not self.directorymanager or not self.directorymanager.supports_authentication():
            user = User("assumed-admin", set(("Superuser",)))
            self._set_session_authinfo(request, callerdeferred, user)
        else:
            d = self.directorymanager.simple_auth(username, password)
            d.addCallback(self._auth_callback, request, callerdeferred)
            d.addErrback(self._auth_errback, request, callerdeferred)
        return callerdeferred

    def _set_session_authinfo(self, request, callerdeferred, user):
        session = get_current_session(request)
        session.user = user
        try:
            session.roles = [Roles.get(r) for r in session.user.role_names]
        except InvalidRoleError, e:
            log.err("Failed to resolve user role: %s" %e)
            callerdeferred.errback(None)
            return
        if session.user.language != None:
            session.language = session.user.language
        else:
            session.language = webserver.lang_from_request(request)
        callerdeferred.callback(None)

    def _auth_callback(self, res, request, callerdeferred):
        if res.status == AuthResult.SUCCESS:
            user = User(res.username, set(res.nox_roles))
            self._set_session_authinfo(request, callerdeferred, user)
        else:
            callerdeferred.errback(res)

    def _auth_errback(self, failure, request, callerdeferred):
        log.err("Failure during authentication: %s" %failure)
        get_current_session(request).expire()
        callerdeferred.errback(None)

    def getInterface(self):
        return str(webauth)

class Capabilities:
    """Stores and provides info on entire set of defined capabilities"""

    def __init__(self):
        self._dict = {}

    def register(self, name, description, immutable_roles=None):
        """Register a capability.

        Capabilities used to control visibity and actions in the UI should
        be registered using this method in the component's install() method.

        Arguments are:
            name: The name of the capability.  This is the string that will
                 be used to refer to the capability subsequently in tests,
                 etc.
            description: A user-readable description of the capability.
                 This will be displayed in the role definition UI to
                 assist the user in determining the appropriate capabilities
                 to give to the role.
            immutable_roles: A list of the names of immutable roles that
                 should have this capability.  Immutable roles are a default
                 set of roles provided by Nicira which the user can not
                 edit.  The capabilities for each of those roles are built
                 from these lists.  This is needed because the capability
                 set may change over time and the editable roles will always
                 assume a role does not have a capability if the user did
                 not specifically set it.  Note it is not neccesary to
                 include the 'Superuser' role in this list as the
                 implementation gurantees that role will have all
                 capabilities."""

        if immutable_roles == None:
            immutable_roles = []
        else:
            for r in immutable_roles:
                if r not in all_immutable_roles:
                    raise InvalidRoleError, "Only roles in webauth.all_immutable_roles are appropiate."

        self._dict[name] = (description, immutable_roles)

    def has_registered(self, name):
        return self._dict.has_key(name)

    def list(self):
        return self._dict.keys()

    def describe(self, name):
        try:
            return self._dict[name][0]
        except KeyError, e:
            raise UnknownCapabilityError, str(name)

    def immutable_roles(self, name):
        try:
            return self._dict[name][1]
        except KeyError, e:
            raise UnknownCapabilityError, str(name)

# Following ensures there is only ever one capabilities manager...
Capabilities = Capabilities()

class Role:
    """Named set of capabilities"""
    def __init__(self, name, immutable=False):
        self.name = name
        self._capabilities = set()
        if immutable:
            self._immutable = False # is overridden below...
            if not  self.name in all_immutable_roles:
                raise InvalidRoleError, "Only roles in webauth.all_immutable_roles can be set immutable."
            for c in Capabilities.list():
                if name in Capabilities.immutable_roles(c):
                    self.add_capability(c)
        self._immutable = immutable

    def capabilities(self):
        return self._capabilities

    def has_capability(self, name):
        return name in self._capabilities

    def has_all_capabilities(self, capability_set):
        return len(capability_set.difference(self._capabilities)) == 0

    def has_anyof_capabilities(self, capability_set):
        return len(capability_set.intersection(self._capabilities)) != 0

    def is_immutable(self):
         return self._immutable

    def add_capability(self, name):
        if not self._immutable:
            if Capabilities.has_registered(name):
                self._capabilities.add(name)
            else:
                raise UnknownCapabilityError, "Name=%s" % name

    def remove_capability(self, name):
        if not self._immutable:
            try:
                self._capabilities.remove(name)
            except KeyError, e:
                pass

class SuperuserRole(Role):
    """Role guaranteed to always have all capabilities"""

    def __init__(self, name):
        Role.__init__(self, name, True)

    def capabilities(self):
        return Capabilities.list()

    def has_capability(self, name):
        return True

    def has_all_capabilities(self, capability_set):
        return True

    def has_anyof_capabilities(self, capability_set):
        return True

class NoAccessRole(Role):
    """Role guaranteed to never have any capabilities"""
    def __init__(self, name):
        Role.__init__(self, name, True)

    def capabilities(self):
        return []

    def has_capability(self, name):
        return False

    def has_all_capabilities(self, capability_set):
        return False

    def has_anyof_capabilities(self, capability_set):
        return False

class Roles:
    """Manages defined roles."""

    def __init__(self):
        self._roles = {}

    def register(self, role):
        self._roles[role.name] = role

    def has_registered(self, role_name):
        return self._roles.has_key(role_name)

    def get(self, role_name):
        try:
            return self._roles[role_name]
        except KeyError, e:
            raise InvalidRoleError(role_name)

    def names(self):
        return [ r.name for r in self._roles.values() ]

    def instances(self):
        return self._roles.values()

# Following ensures there is only ever one roles manager...
Roles = Roles()


class User:
    """User information class"""
    def __init__(self, username=None, role_names=set(), language=None):
        self.username = username
        self.language = language
        self.role_names = role_names

class InvaidAuthSystemError(Exception):
    pass


def requestIsAllowed(request, cap):
    session = get_current_session(request)
    try:
        roles = session.roles
    except AttributeError:
        e = "Forbidding access due to unknown role in requestIsAllowed()"
        log.err(e, system="webauth")
        return False
    if cap is None:
        return True
    for role in roles:
        if role.has_all_capabilities(cap):
            return True
    return False

class MissingTemplateError(Exception):
    pass


class AuthResource(Resource):
    """UI resource class handling authentication.

    This is a subclass of twisted.web.resource.Resource that ensures that
    the current session is associated with a user with the required
    capabilities set to interact with the resource.  It is intended to be
    subclassed in the same way as its twisted parent class.  Similar to
    the way the Twisted Resource class uses the isLeaf class variable,
    subclasses of this class can use two class variables to control
    authentication:

        noUser: (default=False) if True, no authentication will be
                done for this resource.
        required_capabilities: (default=set()) a set object of capabilities
                the user must hold to interact with this resource.
                Capabilites in the list are supplied as strings naming the
                capability and must also be registered with the capabilities
                manager (webauth.Capabilities).  Alternatively, can be a
                dictionary keyed by request method containing a set of
                capabilities for each request method implemented by the
                resource.  If a method is implemented but has not entry
                in the dictionary, it is assummed that no capabilities
                are required.

    Note that the capability checking is primarily a convenience to
    handle the most common cases for simple resources.  For more
    complex situations such as a resources that parses request.postpath
    and thus supports many different URIs, it may be appropriate for the
    method specific render methods to check capabilities directly.

    This class also sets up component specific template search paths,
    and provides a conveience function to render templates with global
    site configuration information passed into the template using the
    contents of the webserver component siteConfig dictionary."""

    noUser = False
    required_capabilities = set()

    def __init__(self, component):
        Resource.__init__(self)
        self.component = component
        self.webserver = component.resolve(str(webserver.webserver))
        self.webauth = component.resolve(str(webauth))

    def getChild(self, name, request):
        if name == '':
            return self
        return Resource.getChild(self, name, request)

    def loginResponse(self, request):
        # This is really not an appropriate response for a real system
        # since it give no clue how to actually get authenticated.
        # Subclasses will typically override this.
        request.setResponseCode(403)
        request.setHeader("Content-Type", "text/plain")
        request.write("Authentication required to access this page.\n".encode("utf-8"))
        request.finish()
        return server.NOT_DONE_YET

    def deniedResponse(self, request):
        return self.loginResponse(request)

    def serverErrorResponse(self, request):
        request.setResponseCode(500)
        request.setHeader("Content-Type", "text/plain")
        request.write("An internal error occured.  Please try again.\n".encode("utf-8"))
        request.finish()
        return server.NOT_DONE_YET
    
    def serverUninitializedResponse(self, request):
        request.setResponseCode(404)
        request.setHeader("Content-Type", "text/plain")
        request.write("Server not yet initialized.  Please try again.\n".encode("utf-8"))
        request.finish()
        return server.NOT_DONE_YET

    def _checkAuth(self, request):
        if self.noUser:       # If resource doesn't require user at all...
            get_current_session(request).requestIsAllowed = requestIsAllowed
            return None
    
        if not self.webauth.is_initialized(): 
            return self.serverUninitializedResponse(request)

        if not self.webauth.requestIsAuthenticated(request):
            get_current_session(request).requestIsAllowed = requestIsAllowed
            return self.loginResponse(request)

        if type(self.required_capabilities) == types.DictionaryType:
            try:
                cs = self.required_capabilities[request.method]
            except KeyError, e:
                cs = None
        else:
            cs = self.required_capabilities

        if cs != None and not isinstance(cs, set):
            e = "Invalid required_capabilities on object: %s" % repr(self)
            log.err(e, system="webauth")
            return (False, self.serverErrorUrl(request))

        if not requestIsAllowed(request, cs):
            return self.deniedResponse(request)

        return None

    def render(self, request):
        session = get_current_session(request)
        authresponse = self._checkAuth(request)
        if authresponse != None:
            return authresponse
        else:
            return Resource.render(self, request)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return webauth(ctxt)

    return Factory()
