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
#ifndef lavi_hostflow_HH
#define lavi_hostflow_HH

#include "component.hh"
#include "config.h"
#include "route/flowroute_record.hh"
#include "network_graph.hh"
#include "lavi_flows.hh"
#include "lavi_host2sw.hh"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/** \brief Default to sending remove messages or not
 *
 * Really wants a way of checking if any other lavi_flows components
 * are present, since remove affects everyone.  Or maybe another components
 * for flow removal is the right thing.
 */
#define LAVI_HOSTFLOW_SEND_REMOVE false

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  /** \brief lavi_hostflow: Shows flows to/from host
   * \ingroup noxcomponents
   * 
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "request", "subscribe", "unsubscribe">
   *   "node_type" : <string | "all", "host">
   * }
   * </PRE>
   * while reply should the following form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "add", "delete">
   *   "flow_type" : "host"
   *   "flow_id" : <string: use cookie (if cookie==0, use hash code) of flow>
   *   "flows" : [ { "src type": "host",
   *                 "src id" : <string>,
   *                 "dst type": "switch",
   *                 "dst id" : <node id>,
   *                 "dst port" : <string>
   *               }, ...]
   * }
   * </PRE>
   * Note that flow_id is globally unique.
   * Delete messages do not include the path of the flow.
   *
   * @author ykk
   * @date August 2010
   */
  class lavi_hostflow
    : public lavi_flows
  {
  public:
    /** \brief Indicate if remove message should be sent
     */
    bool send_remove_msg;

    /** \brief Constructor of lavi_hostflow.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    lavi_hostflow(const Context* c, const json_object* node)
      : lavi_flows(c, node)
    {}
    
    /** \brief Configure lavi_hostflow.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Function to send list.
     *
     * @param stream message stream to send list to
     */
    void send_list(const Msg_stream& stream);

    /** \brief Handle flow/route changes.
     * @param e event
     * @return CONTINUE
     */
    Disposition handle_flow_route(const Event& e);

    /** \brief Start lavi_hostflow.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of lavi_hostflow.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_hostflow*& component);

  private:
    /** \brief Reference to flow/route record
     */
    flowroute_record* frr;
    /** \brief Reference to host2sw
     */
    lavi_host2sw* lh2s;

    /** \brief Function to send flow to socket
     * @param flow flow having route
     * @param rte route to send
     * @param stream socket to send to
     * @param add adding or removing flow
     */
    void send_flow_list(const Flow& flow, const network::route& rte,
			const Msg_stream& stream, bool add=true);
 
    /** \brief Pack hop to and from hosts into JSON object
     * @param ja pointer to json_array
     * @param flow flow to determine source and destination
     * @param rte route
     * @param first first hop (from source)
     */
    void get_host_route(json_array* ja, const Flow& flow,
			const network::route& rte, 
			bool first=true);
  };
}

#endif
