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
#ifndef simplerouting_HH
#define simplerouting_HH

/** Post in flowroute_record or not.
 */
#define SIMPLEROUTING_POST_RECORD_DEFAULT false

#include "component.hh"
#include "config.h"
#include "hoststate/hosttracker.hh"
#include "routeinstaller.hh"
#include "flowroute_record.hh"

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

  /** \brief simplerouting: routing with dependencies on authenticator
   * \ingroup noxcomponents
   *
   * Can choose to set flow installed into flowroute_record with arguments
   * <PRE>
   *    simplerouting=postrecord=true
   * </PRE>
   * or
   * <PRE>
   *    simplerouting=postrecord=false
   * </PRE>
   *
   * 'cos this is easier to do than understand why authenticator does not
   * register some mac addresses 
   *
   * @author ykk
   * @date July 2010
   */
  class simplerouting
    : public Component 
  {
  public:
    /** \brief Post flow record or not
     */
    bool post_flow_record;

    /** \brief Constructor of simplerouting.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    simplerouting(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure simplerouting.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Handle packet in to route
     * @param e packet in event
     * @return CONTINUE always
     */
    Disposition handle_pkt_in(const Event& e);

    /** \brief Start simplerouting.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of simplerouting.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    simplerouting*& component);

  private:
    /** Reference to routeinstaller
     */
    routeinstaller* ri;
    /** Reference to hosttracker
     */
    hosttracker* ht;
    /** Reference to flowroute_record
     */
    flowroute_record* frr;
  };
}

#endif
