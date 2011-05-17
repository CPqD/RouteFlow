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
%module "nox.coreapps.pysimple_cc_py_app"


%{
#include "simple_cc_py_proxy.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%include "simple_cc_py_proxy.hh"

%pythoncode
%{
  from nox.lib.core import Component

    class pysimple_cc_py_app(Component):
      """
        An adaptor over the C++ based Python bindings to
        simplify their implementation.
      """  
      def __init__(self, ctxt):
        self.pscpa = simple_cc_py_proxy(ctxt)

      def configure(self, configuration):
        self.pscpa.configure(configuration)

      def install(self):
        pass

      def getInterface(self):
        return str(pysimple_cc_py_app)

      # --
      # Expose additional methods here!
      # --


  def getFactory():
        class Factory():
            def instance(self, context):
                        
                return pysimple_cc_py_app(context)

        return Factory()
%}
