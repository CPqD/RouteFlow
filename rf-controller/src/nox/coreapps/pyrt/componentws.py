from nox.webapps.webservice  import webservice
from nox.lib.config   import version

from nox.lib.core import *

import simplejson

#
# Verifies that component name exists
#
class WSPathExistingComponent(webservice.WSPathComponent):
    def __init__(self, component):
        webservice.WSPathComponent.__init__(self)
        self._component = component

    def __str__(self):
        return "<component name>"

    def extract(self, pc, data):
        if pc == None:
            return webservice.WSPathExtractResult(error="End of requested URI")

        components = self._component.ctxt.get_kernel().get_all()
        for component in components:
            if component.get_name() == pc:
                return webservice.WSPathExtractResult(pc)
        e = "Unknown component '%s'" % pc
        return webservice.WSPathExtractResult(error=e)


class componentws(Component):
    """Web service interface to component runtime"""

    cstate = {}
    cstate[0] = 'NOT_INSTALLED'
    cstate[1] = 'DESCRIBED'
    cstate[2] = 'LOADED'
    cstate[3] = 'FACTORY_INSTANTIATED'
    cstate[4] = 'INSTANTIATED'
    cstate[5] = 'CONFIGURED'
    cstate[6] = 'INSTALLED'
    cstate[7] = 'ERROR'


    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def _get_nox_components(self, request, arg):
        components = self.ctxt.get_kernel().get_all()
        cdict = {} 
        cdict['identifier'] = 'name'
        cdict['items'] = [] 
        for component in components:
            comp = {}
            comp['name']    = component.get_name()
            comp['version'] = version 
            comp['uptime'] = self.ctxt.get_kernel().uptime()
            comp['state']   = componentws.cstate[component.get_state()]
            comp['required_state']  = componentws.cstate[component.get_required_state()]
            cdict['items'].append(comp)

        return simplejson.dumps(cdict)

    # not implemented yet
    def _get_component_uptime(self,request,arg): 
      return simplejson.dumps("")
    
    # not implemented yet
    def _get_component_version(self,request,arg): 
      return simplejson.dumps("")

    def _get_nox_component_status(self, request, arg):
        components = self.ctxt.get_kernel().get_all()
        cname = arg['<component name>']
        for component in components:
            if component.get_name() == cname:
                cdict = {}
                cdict['name']   = component.get_name()
                cdict['state'] = component.get_state()
                cdict['required_state'] = component.get_required_state()
                return simplejson.dumps(cdict)

    def install(self):
        
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request

        # /ws.v1/nox
        noxpath = ( webservice.WSPathStaticString("nox"), )

        # /ws.v1/nox/components
        noxcomponentspath = noxpath + \
                        ( webservice.WSPathStaticString("components"), )
        reg(self._get_nox_components, "GET", noxcomponentspath,
            """Get list of nox components and their status""")

        # /ws.v1/component/<component name>
        noxcomponentnamepath = noxpath + \
                        ( webservice.WSPathStaticString("component"), 
                        WSPathExistingComponent(self))

        # /ws.v1/component/<component name>/status
        noxcomponentstatus = noxcomponentnamepath + \
                        ( webservice.WSPathStaticString("status"), )
        reg(self._get_nox_component_status, "GET", noxcomponentstatus,
            """Get status for given nox component""")

        # /ws.v1/component/<component name>/version
        # XXX Currently just return nox version 
        noxcomponentversion = noxcomponentnamepath + \
                        ( webservice.WSPathStaticString("version"), )
        reg(self._get_component_version, "GET", noxcomponentversion,
            """Get version for given nox component""")

        # /ws.v1/component/<component name>/uptime
        # XXX Currently just return nox uptime
        noxcomponentuptime = noxcomponentnamepath + \
                        ( webservice.WSPathStaticString("uptime"), )
        reg(self._get_component_uptime, "GET", noxcomponentuptime,
            """Get uptime for given nox component""")

    def getInterface(self):
        return str(componentws)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return componentws(ctxt)

    return Factory()
