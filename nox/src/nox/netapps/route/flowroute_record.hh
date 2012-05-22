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
#ifndef flowroute_record_HH
#define flowroute_record_HH

#include "component.hh"
#include "config.h"
#include "flow.hh"
#include "network_graph.hh"

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

  /** \ingroup noxevents
   * \brief Structure to hold route record change
   */
  struct Flow_route_event : public Event
  {
    /** \brief Type of flow-route event
     */
    enum type
    {
      ADD,
      REMOVE,
      MODIFY
    };

    /** \brief Constructor
     * @param flow_ flow to record
     * @param rte_ route of flow to record
     * @param eventType_ type of event
     */
    Flow_route_event(const Flow& flow_, const network::route& rte_,
		     enum type eventType_):
      Event(static_get_name()), rte(rte_), flow(flow_), eventType(eventType_)
    {}

    /** For use within python.
     */
    Flow_route_event() : Event(static_get_name()) 
    { }

    /** Static name required in NOX.
     */
    static const Event_name static_get_name() 
    {
      return "Flow_route_event";
    }

    /** Reference to route
     */
    network::route rte;
    /** Reference to flow
     */
    Flow flow;
    /** Type of event
     * @see #type
     */
    enum type eventType;
  };

  /** \brief flowroute_record: record of flow and associated route
   * \ingroup noxcomponents
   * 
   * Does not handle detection of flow and route nor its expiration/deletion.
   * Just a component that stores the mapping.
   *
   * Post Flow_route_event upon change
   *
   * @author ykk
   * @date June 2010
   */
  class flowroute_record
    : public Component 
  {
  public:
    /** \brief Map of flows and route
     */
    hash_map<Flow, network::route> routeMap;

    /** \brief Constructor of flowroute_record.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    flowroute_record(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Record flow and route
     * Update if flow existing else create new entry
     *
     * @param flow flow to record
     * @param rte route to record
     * @param throwEvent indicate if event should be posted
     */
    void set(const Flow& flow, const network::route& rte, 
	     bool throwEvent=true);

    /** \brief Remove flow-route route
     * Update if flow existing else create new entry
     *
     * @param flow flow to remove
     * @param throwEvent indicate if event should be posted
     * @return if event is removed
     */
    bool unset(const Flow& flow, bool throwEvent=true);

    /** \brief Get flows/route directed at host
     * @param host ethernet address of host
     * @return list of flows and routes associated with host
     */
    list<pair<Flow,network::route> > getFlow2Host(ethernetaddr host);

    /** \brief Configure flowroute_record.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start flowroute_record.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of flowroute_record.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    flowroute_record*& component);

  private:
  };
}

#endif
