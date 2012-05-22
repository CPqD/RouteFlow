/* Copyright 2008 (C) Nicira, Inc.
 * Copyright 2009 (C) Stanford University.
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
#ifndef simple_cc_app_HH
#define simple_cc_app_HH

#include "component.hh"
#include "config.h"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  /** \brief simple_cc_app
   * \ingroup noxcomponents
   * 
   * @author
   * @date
   */
  class simple_cc_app
    : public Component 
  {
  public:
    /** \brief Constructor of simple_cc_app.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    simple_cc_app(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure simple_cc_app.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start simple_cc_app.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of simple_cc_app.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    simple_cc_app*& component);

  private:

  };
}

#endif
