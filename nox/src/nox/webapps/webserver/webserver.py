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
import base64
import os
import logging
import mimetypes

from OpenSSL import crypto, SSL

from twisted.web import static, server, resource
from twisted.internet import defer, reactor, ssl
from twisted.internet.ssl import ContextFactory
from twisted.web.util import redirectTo

from nox.lib.config   import buildnr
from nox.coreapps.pyrt.pycomponent import *
from nox.lib.core import *


lg = logging.getLogger('webserver')

supported_languages = ( "en", "ja" )

def lang_from_request(request):
    languages = []
    if request.getHeader("Accept-Language") is None:
        return "en"
    for l in request.getHeader("Accept-Language").split(","):
        t = l.split(";q=")
        name = t[0].strip()
        if len(t) > 1:
            qvalue = float(t[1])
        else:
            qvalue = 1.0
        languages.append((name, qvalue))
    languages.sort(key=lambda x: x[1], reverse=True)
    lang = "en"
    base_lang = lang
    if len(languages) > 0:
        t = languages[0][0].split("-")
        if len(t) > 1:
            lang = t[0].lower() + "_" + t[1].upper()
        else:
            lang = t[0].lower()
        base_lang = t[0].lower()

    if base_lang not in supported_languages:
        lang="en"   # This had better always be supported!
    return lang

# Twisted hardcodes the string TWISTED_SESSION as the cookie name.
# This hack let's us append a Nicira string to that cookie name.
# In the future, we may want to override Request.getSession()
# as implemented in twisted/web/server.py to completely remove
# TWISTED_SESSION from the cookie name.
# NOTE: spaces in the cookie name are incompatible with Opera
def get_current_session(request):
        old_sitepath = request.sitepath
        request.sitepath = [ "Nicira_Management_Interface" ]
        session = request.getSession()
        request.sitepath = old_sitepath
        return session

def redirect(request, uri):
    # TBD: make handle child links automatically, normalize URI, etc.
    return redirectTo(uri, request)
        
  
class UninitializedRes(resource.Resource):
            def __init__(self):
              resource.Resource.__init__(self)
              self.render_POST = self.render_GET

            def getChild(self, name, request):
              return self

            def render_GET(self, request):
                  request.setResponseCode(500)
                  request.setHeader("Content-Type", "text/html")
                  msg = """<html><head> 
                          <meta http-equiv='refresh' content='1'> 
                          </head> <body> Initializing web server. Please wait...
                          </body></html>"""
                  request.write(msg)
                  request.finish()
                  return server.NOT_DONE_YET


class NoxStaticSrvr(static.File):
    """Subclass of twisted file that sets cache-control as we want."""

    # TBD: - This may be an appropriate place to do on-the-fly gzip
    # TBD:   encoding if the client supports that as well...

    # cacheControl is only settable for the server as a whole...
    cacheControl = "max-age=" + str(60*60*24*365)

    def __init__(self, *arg, **kwarg):
        static.File.__init__(self, *arg, **kwarg)

    def render(self, request):
        request.setHeader('cache-control', self.cacheControl)
        return static.File.render(self, request)


## \ingroup noxcomponents
## Webserver component.
class webserver(Component):
    """Webserver component"""
    STATIC_FILE_BASEPATH = "nox/webapps/webserver/www/"

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.root = None
        self.default_uri = None

        self.defaults = {
            "network_name" : ["Local"],
            "port" : [ int(80) ],
            "ssl_certificate" : [ u"" ],
            "ssl_privatekey" : [ u"" ],
            "ssl_port" : [ int(443) ],
            }

        self.using_ssl = False

        self.port = None      # Bound Twisted port object
        self.ssl_port = None  # Bound Twisted port object
        #self.closing_ports =[]# Bound Twisted ports waiting for closing
        self.restart = 0      # Web server reconfiguration request indicator
        self.reconfiguring = False # Reconfiguring at the moment
        self.siteConfig = {}  # Dictionary of attribute/values that will be
                              # passed to the mako templates.  Note this is
                              # not intended as a catchall for everything to
                              # be passed to templates but rather to cover
                              # site structure information like the sections
                              # and subsections since they are dynamically
                              # determined.
        self.siteConfig['webserver'] = self
        self.siteConfig['staticBase'] = "/static/" + str(buildnr)
        # this siteName is overridden by components like 'snackui', 'vnetui'
        # we should not serve up any pages until our
        # authentication infrastructure has been initialized
        # (this does not happen until bootstrap-complete)
        self.authui_initialized = False

        # branding defaults
        # A specific type of branding is implemented 
        # by having a 'branding component' resolve this component
        # and change these defaults.  
        self.siteConfig["branding_css_file"] = ""
        self.siteConfig["branding_css_class"] = ""
        self.siteConfig["app_name_short"] = "Network Controller"
        self.siteConfig["app_name_long"] = "Nicira Network Controller"

    def ssl_enabled(self):
        return using_ssl

    def install_section(self, res, hidden=False):
        """Install a new top-level section of the site.

        Top-level sections are represented by the links/icons that appear
        in the toolbar under the main site banner.  They should represent
        significant areas of functionality and new ones should not be added
        lightly.  Typically functionality will be added within the existing
        sections instead.

        Arguments are :
             res: Twisted Resource object subclass handling web requests for
                  the site section.  Typically this should be subclassed from
                  UISection to ensure all required information is available."""
        if not hidden:
            try:
                self.siteConfig["sections"].append(res)
            except KeyError, e:
                self.siteConfig["sections"] = [ res ]
        self.root.putChild(res.section_name, res)
        if self.default_uri == None:
            self.default_uri = "/" + res.section_name

    def install_resource(self, path, res):
        """Install a resource on a specific URI path.

        This is not intended for typical uses.  Typically components should
        add their information to the UI through install_section or one of
        the APIs provided by the existing sections (for example monitorui,
        policyui, etc.)

        Currently the only known legitimate user of this call is the authui
        module which uses it to install the pages used for doing login
        and logout."""
        if path[0] != "/":
            raise ImplementationError, "The path parameter must start with /"
        path_components = path.split("/")[1:]
        if path_components[-1] == "":
            path_components = path_components[:-1]
        parent = self.root
        for pc in path_components[:-1]:
            parent = parent.children[pc]
            if parent.isLeaf:
                raise ImplementationError, "Path is already handled."
        parent.putChild(path_components[-1], res)

    def configure(self, conf):

         def merge(args, p):
             """
             Merges the options defined on the command line with the
             database stored parameters.  Command line options override
             the database ones.
             """

             def load_self_signed_cert(p):
                import os
                import nox.lib.config
                cert_file = 'etc/noxca.cert'
                key_file  = 'etc/noxca.key.insecure'
                if not os.path.isfile(cert_file):
                    if not os.path.isfile(nox.lib.config.pkgsysconfdir+"/noxca.cert"):
                        lg.error('Could not find self signed cert.  Disabling ssl')
                        return
                    else:    
                        cert_file = nox.lib.config.pkgsysconfdir+"/noxca.cert"
                if not os.path.isfile(key_file):
                    if not os.path.isfile(nox.lib.config.pkgsysconfdir+"/noxca.key.insecure"):
                        lg.error('Could not find self signed key.  Disabling ssl')
                        return
                    else:    
                        key_file = nox.lib.config.pkgsysconfdir+"/noxca.key.insecure"
                
                p['ssl_certificate'] = [base64.b64encode(open(cert_file).read())]
                p['ssl_privatekey']  = [base64.b64encode(open(key_file).read())]


             # if ssl_certificate not set in properties, use default
             # self signed certs
             if  p['ssl_certificate'][0] == u'':
                load_self_signed_cert(p)

             for a in args:
                 try:
                     def update_iff_changed(p, key, value):
                         if not p[key][0] == value: p[key] = value

                     arr = a.split("=")
                     assert len(arr) == 2,  "params must be 'key=value' pairs"
                     #if arr[0] == "privatekey":
                     #    assert os.path.exists(arr[1]), "privatekey file must exist"
                     #    update_iff_changed(p, 'ssl_privatekey_fname', arr[1])
                     #elif arr[0] == "cert":
                     #    assert os.path.exists(arr[1]), "private_key file must exist"
                     #    update_iff_changed(p, 'ssl_certificate_fname', arr[1])
                     if arr[0] == "ssl_port":
                         update_iff_changed(p, 'ssl_port', int(arr[1]))
                     elif arr[0] == "port":
                         update_iff_changed(p, 'port', int(arr[1]))
                 except Exception, e:
                     lg.warn("config error with '%s' : %s " % (a,str(e)))


         # XXX This should be abstracted out eventually.  Nothing in core
         # should depend on ext
         try:
           from nox.ext.apps.tstorage import TStorage
           from nox.ext.apps.configuration.properties import Properties
           self.storage = self.resolve(TStorage)

           # Store the changes to the database, if necessary.
           p = Properties(self.storage, 'nox_config', self.defaults)
           return p.begin().\
               addCallback(lambda ign: merge(conf['arguments'], p)).\
               addCallback(lambda ign: p.commit())
         except ImportError, e:
           # if properties don't exist, use
           # system defaults or CLAs 
           merge(conf['arguments'], self.defaults)

    def install(self):
        # Setup twisted.web instance for pages

        class RootRes(resource.Resource):
            def __init__(self, component):
                resource.Resource.__init__(self)
                self.component = component

            def getChild(self, name, request):
                if not self.component.authui_initialized:
                    return UninitializedRes()

                if name == '':
                    return self
                return resource.Resource.getChild(self, name, request)

            def render_GET(self, request):
                if self.component.default_uri == None:
                    return "<html><head><title>Internal Error</title></head><body><h1>Internal Error</h1><p>Site is not properly configured.</p></body></html>"
                return redirectTo(self.component.default_uri, request)

        self.root = RootRes(self)
        self.static_root = NoxStaticSrvr(self.STATIC_FILE_BASEPATH)
        # If this is a developer build, we don't want to cache.
        if buildnr == 0:
            NoxStaticSrvr.cacheControl="no-cache, no-store, must-revalidate"
        self.root.putChild("static", self.static_root)

        return self.reconfigure()

    def configuration_changed(self):
        """
        Configuration has changed in the database.  Enqueue a
        reconfiguring cycle.
        """

        self.restart += 1
        self.tickle()

    def tickle(self):
        # Initiate reconfigure only, if there's none going on.
        if not self.reconfiguring and self.restart > 0:
            self.restart = 0
            self.reconfiguring = True
            reactor.callLater(0, self.reconfigure)

    def restart_error(self, failure):
        lg.exception(failure)
        delay = 3
        lg.debug('Retrying to reconfigure UI web server in %d seconds.', delay)

        self.reconfiguring = False
        self.restart += 1
        reactor.callLater(delay, self.tickle)

    def reconfigure(self):
        self.reconfiguring = True

        # XXX This should be abstracted out eventually.  Nothing in core
        # should depend on ext
        try:
          from nox.ext.apps.configuration.properties import Properties
          p = Properties(self.storage, 'nox_config')
          return p.load(self.configuration_changed).\
              addCallback(self.reconfigure_listeners, p).\
              addErrback(self.restart_error)
        except ImportError, e:
           # if properties don't exist use the system defaults 
           self.reconfigure_listeners(0, self.defaults)

    #def close_ports(self, port_to_close=None):
    #    remaining_ports_to_close = []
    #
    #    for port in self.closing_ports:
    #        if port_to_close == None or port_to_close == port.port:
    #            lg.debug('Closing the listener: %s', port)
    #            port.stopListening()
    #        else:
    #            remaining_ports_to_close.append(port)
    #
    #    self.closing_ports = remaining_ports_to_close

    def reconfigure_listeners(self, ignore, p):
        # Store the current configuration
        # Any data in siteConfig can be exposed to a web user in
        # a mako traceback.  We do not want sensitive data like the 
        # SSL private key stored there.  Thus, just copy the specified
        # items into siteConfig
        for key in p.keys():
            if key in [ "network_name", "port", "ssl_port" ] : 
              list = p[key]
              if len(list) > 0: self.siteConfig[key] = list[0]

        self.ssl_reconfigured = False

        def close_https(ign):
            if self.ssl_port:
                ssl_port = self.ssl_port
                self.ssl_port = None
                return ssl_port.stopListening()

        def restart_https(ign):
            self.ssl_reconfigured = True

            try:
                sslContext = UIOpenSSLContextFactory(\
                    base64.b64decode(p['ssl_privatekey'][0]),
                    base64.b64decode(p['ssl_certificate'][0]))
                if not p['ssl_port'][0] == 0:
                    port = reactor.listenSSL(p['ssl_port'][0],
                                             server.Site(self.root),
                                             contextFactory=sslContext)
                    self.using_ssl = True
                else:
                    port = None
                    self.using_ssl = False

                self.ssl_privatekey = p['ssl_privatekey'][0]
                self.ssl_certificate = p['ssl_certificate'][0]
                self.ssl_port = port

            except Exception, e:
                self.using_ssl = False
                lg.error("Error starting HTTPS server: %s" % str(e))

        def close_http(ign):
            if self.port:
                return self.port.stopListening()
            return None

        def restart_http(ign):
            self.port = None

            try:
                if not p['port'][0] == 0:
                    if self.using_ssl:
                        class RedirectRes(resource.Resource):
                            def __init__(self, component):
                                resource.Resource.__init__(self)
                                self.component = component
                    
                            def getChild(self, name, request):
                                if not self.component.authui_initialized:
                                    return UninitializedRes()
                    
                                return self
                    
                            def render_GET(self, request):
                                hostname = request.getRequestHostname()
                                if not self.component.ssl_port:
                                    # SSL port is not enabled anymore!
                                    # Ask the browser to close the
                                    # connection, and reconnect. There
                                    # should be a new listener
                                    # waiting, w/o redirection.
                                    request.setHeader('connection', 'close')
                                    new_uri = 'http://%s:%d%s' % \
                                        (hostname, self.component.port.port, 
                                         request.uri)
                                    return redirectTo(new_uri, request)
                                else:
                                    new_uri = 'https://%s:%d%s' % \
                                        (hostname, 
                                         self.component.ssl_port.port, 
                                         request.uri)
                                    return redirectTo(new_uri, request)
                    
                        root = RedirectRes(self)
                    else:
                        root = self.root
                    self.port = reactor.listenTCP(p['port'][0], 
                                                  server.Site(root))
                else:
                    self.port = None

            except Exception, e:
                lg.error("Error starting HTTP server: %s" % str(e))

        def complete(ign):

            # Register MimeTypes me must have properly mapped
            self.static_root.contentTypes.update({
                    ".htc" : "text/x-component"
                    })

            # TBD: Get cometd server running in one server or another...
            self.reconfiguring = False

        d = defer.succeed(None)

        # Rebind the port only, if there's a change
        if (self.ssl_port == None and \
                (not p['ssl_port'][0] == 0 and \
                     not p['ssl_privatekey'][0] == '' and \
                     not p['ssl_certificate'][0] == '')) or \
                (not self.ssl_port == None and \
                     (not self.ssl_port.port == p['ssl_port'][0] or \
                          not self.ssl_privatekey == p['ssl_privatekey'][0] or \
                          not self.ssl_certificate == p['ssl_certificate'][0])):
            d.addCallback(close_https)
            d.addCallback(restart_https)

        # Rebind the port only, if there's a change

        def restart_http_if_necessary(ign):
            if self.ssl_reconfigured or \
                    (self.port == None and not p['port'][0] ==0) or \
                    (not self.port == None and \
                         not self.port.port == p['port'][0]):
                return defer.succeed(None).\
                    addCallback(close_http).\
                    addCallback(restart_http)

        return d.addCallback(restart_http_if_necessary).\
            addCallback(complete)

    def getInterface(self):
        return str(webserver)

class UIOpenSSLContextFactory(ContextFactory):
    def __init__(self, privatekey, certificate):
        self.pkey = crypto.load_privatekey(crypto.FILETYPE_PEM, privatekey, '')
        self.cert = crypto.load_certificate(crypto.FILETYPE_PEM, certificate)
        self.cacheContext()

    def cacheContext(self):
        ctx = SSL.Context(SSL.SSLv23_METHOD)
        ctx.use_privatekey(self.pkey)
        ctx.use_certificate(self.cert)
        self._context = ctx

    def __getstate__(self):
        d = self.__dict__.copy()
        del d['_context']
        return d

    def __setstate__(self, state):
        self.__dict__ = state
        self.cacheContext()

    def getContext(self):
        return self._context

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return webserver(ctxt)

    return Factory()
