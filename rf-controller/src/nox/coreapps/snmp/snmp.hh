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
#ifndef snmp_HH
#define snmp_HH

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

  /** \brief snmp: SNMP get/set support in NOX
   * \ingroup noxcomponents
   * 
   * @author ykk
   * @date June 2010
   */
  class snmp
    : public Component 
  {
  public:
    /** \brief Constructor of snmp.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    snmp(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure snmp.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start snmp.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of snmp.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    snmp*& component);

  private:

  };

  /** \page snmp SNMP Support in NOX
   * 
   * The support for SNMP uses NetSNMP, which is commonly distributed
   * in Linux distros.  There are two parts to the SNMP support.
   * <OL>
   * <LI> Trap/Inform which allows events to be notificated to the controller.
   *     This is supported using snmptrapd, which can duplicates and proxy trap
   *     or inform messages to appropriate processors.  See snmptrapd.<BR>
   *
   *     This is proxied to NOX using JSONMsg_event with the following JSON message.
   * <PRE>
   * {
   *    "type" : "snmptrap",
   *    "content" : [ [ <string>, ...],
   *                  ...]
   * }
   * </PRE>
   *      where content is array of lines which is further tokenized in Python
   *      script \ref traphandler.
   * </LI>
   * <LI> Get/set allows controllers to poll and configure devices via SNMP.
   *     An asynchronous call is used in NOX during the NOX's threading model.
   * </LI>
   * </OL>
   */
}

#endif
