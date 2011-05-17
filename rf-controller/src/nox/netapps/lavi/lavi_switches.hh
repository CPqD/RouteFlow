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
#ifndef lavi_switches_HH
#define lavi_switches_HH

#include "component.hh"
#include "config.h"
#include "lavi_nodes.hh"
#include "networkstate/datapathmem.hh"

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

  /** \brief lavi_switches: Expose switch join and leave
   * \ingroup noxcomponents
   * 
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "request", "subscribe", "unsubscribe">
   *   "node_type" : <string | "all", "switch">
   * }
   * </PRE>
   * while reply should the following form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "add", "delete">
   *   "node_type" : "switch"
   *   "node_id" : [ <string in hex>, ...]
   * }
   * </PRE>
   * Note that node_id is only unique per node_type, i.e., 
   * only node_type:node_id is globally unique.
   *
   * @see lavi_nodes
   * @see datapathmem
   * @author ykk
   * @date May 2010
   */
  class lavi_switches
    : public lavi_nodes
  {
  public:
    /** \brief Constructor of lavi_switches.
     *
     * @param c context
     * @param node configuration (JSON object)
     */
    lavi_switches(const Context* c, const json_object* node)
      : lavi_nodes(c, node)
    {}
    
    /** \brief Configure lavi_switches.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Function to track switch add.
     * @param e switch join event
     * @return CONTINUE always
     */
    Disposition handle_switch_add(const Event& e);

    /** \brief Function to track switch deletion.
     * @param e switch leave event
     * @return CONTINUE always
     */
    Disposition handle_switch_del(const Event& e);

    /** \brief Function to send list.
     *
     * @param stream message stream to send list to
     */
    void send_list(const Msg_stream& stream);

    /** \brief Start lavi_switches.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of lavi_switches.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_switches*& component);

  private:
    /** \brief Send list of switches
     * @param stream socket to send list over
     * @param dpid_list list of datapathid to send
     * @param add indicate if add or delete
     */
    void send_swlist(const Msg_stream& stream, list<uint64_t> dpid_list,
		     bool add=true);

    /** \brief Reference to datapathmem.
     */
    datapathmem* dpm;
  };
}

#endif
