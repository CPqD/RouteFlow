/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
%module "nox.netapps.flow_fetcher.pyflow_fetcher"


%{
#include "flow_fetcher_proxy.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%include "flow_fetcher_proxy.hh"

%pythoncode
%{
  from nox.lib.core import Component

  class flow_fetcher_app(Component):
    """
      An adaptor over the C++ based Python bindings to
      simplify their implementation.
    """  
    def __init__(self, ctxt):
      Component.__init__(self, ctxt)

    def getInterface(self):
      return str(flow_fetcher_app)

    def configure(self, configuration):
      pass

    def install(self):
      pass

    # This is the function that you want.
    #
    # Flows satisfying the predicate in 'request' will be fetched from
    # the switch with datapath id 'dpid'.  When the fetch completes
    # (either successfully or with an error), 'cb' will be invoked.
    # (But 'cb' will not be invoked before fetch() returns.)
    #
    # The 'request' takes the form of a dictionary representing an
    # ofp_flow_stats_request, e.g.:
    #   {'table_id': ...table id...,
    #    'match': {'in_port': 0,
    #              'dl_src': 0x002320ed7e64,
    #              'dl_dst': 0x002320ed7e65,
    #              'dl_vlan': 0x234,
    #              'dl_vlan_pcp': 7,
    #              'dl_type': 0x0800,
    #              'nw_proto': 17,
    #              'nw_src': 0x525400123502,
    #              'nw_dst': 0x525400123503,
    #              'tp_src': 1234,
    #              'tp_dst': 5678}}
    # where every member is optional and by its absence indicates a wildcard
    # across that field.  See the definition of
    # from_python<ofp_flow_stats_request> in pyglue.cc for details.
    #
    # Returns a flow fetcher object whose state may be queried to find out the
    # results of the fetch operation or to cancel the operation.  A call to the
    # callback indicates that the flow fetch operation is complete and that the
    # flow fetcher's state will no longer change.
    #
    # The returned flow fetcher object must be saved somewhere or the operation
    # will be canceled.  Typical usage is something like:
    #   ff = ffa.fetch(dpid, flow_stats_request, lambda: report_results(ff))
    # where ffa is the flow_fetcher_app obtained via resolve, e.g. from a
    # Component subclass's install method:
    #   ffa = self.resolve(flow_fetcher_app)
    #
    # See test.py in this directory for example usage.
    def fetch(self, dpid, request, cb):
      return flow_fetcher(self.ctxt, dpid, request, cb)

  class flow_fetcher:
    def __init__(self, ctxt, dpid, request, cb):
      self.pff = Flow_fetcher_proxy(ctxt, dpid, request, cb)

    def cancel(self):
      self.pff.cancel()

    def get_status(self):
      return self.pff.get_status()

    def get_flows(self):
      return self.pff.get_flows()

  def getFactory():
        class Factory():
            def instance(self, context):
                return flow_fetcher_app(context)

        return Factory()
%}
