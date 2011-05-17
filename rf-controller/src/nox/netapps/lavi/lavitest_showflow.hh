/* Copyright 2010 (C) Stanford University.
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
#ifndef lavitest_showflow_HH
#define lavitest_showflow_HH

#include "component.hh"
#include "config.h"
#include "route/simplerouting.hh"

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

  /** \brief lavitest_showflow: test case for showing flows using lavi
   * \ingroup noxcomponents
   * 
   * @see lavi_switches
   * @see lavi_swlinks
   * @see lavi_networkflow
   * @see lavi_host2sw
   * @see lavi_hosts
   * @see simplerouting
   * @author ykk
   * @date July 2010
   */
  class lavitest_showflow
    : public Component 
  {
  public:
    /** \brief Constructor of lavitest_showflow.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    lavitest_showflow(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure lavitest_showflow.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Show all flow mod
     * @param e flow route event
     * @return CONTINUE always
     */
    Disposition handle_flow_route(const Event& e);

    /** \brief Start lavitest_showflow.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of lavitest_showflow.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavitest_showflow*& component);

  private:
    /** \brief Reference to simple routing
     */
    simplerouting* sr;
  };
}

#endif
