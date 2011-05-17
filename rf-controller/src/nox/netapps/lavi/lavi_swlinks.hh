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
#ifndef lavi_swlinks_HH
#define lavi_swlinks_HH

#include "component.hh"
#include "config.h"
#include "lavi_links.hh"
#include "topology/topology.hh"
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
  using namespace vigil::applications;

  /** \brief lavi_swlinks: Expose links between switches
   * \ingroup noxcomponents
   * 
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi",
   *   "command" : <string | "request", "subscribe", "unsubscribe">,
   *   "link_type" : <string | "all", "sw2sw">,
   * </PRE>
   * with optional parameters
   * <PRE>
   *   "filter" : [ { "node_type" : <string>,
   *                  "node_id" : <string> }, 
   *               ... ]
   * }
   * </PRE>
   * where link will be presented in result if either end matches all
   * of the filter specified.  Reply should the following form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "add", "delete">
   *   "link_type" : "sw2sw"
   *   "links" : [ { "src type" : <string>,
   *                 "src id" : <string>,
   *                 "src port" : <string>,
   *                 "dst type" : <string>,
   *                 "dst id" : <string>,
   *                 "dst port" : <string>
   *               }, ...]
   * }
   * </PRE>
   *
   * @see lavi_links
   * @see datapathmem
   * @see vigil::applications::Topology
   * @author ykk
   * @date June 2010
   */
  class lavi_swlinks
    : public lavi_links
  {
  public:
    /** \brief Structure to hold information about swlink
     *
     * @author ykk
     * @date Juen 2010
     */
    struct swlink
    {
      /** Type of source node
       */
      string src_nodetype;
      /** Id of source node
       */
      string src_nodeid;
      /** Port of source node
       */
      string src_port;
      /** Type of destination node
       */
      string dst_nodetype;
      /** Id of destination node
       */
      string dst_nodeid;
      /** Port of destination node
       */
      string dst_port;

      /** \brief Constructor
       * @param nt1 node 1 type (src)
       * @param ni1 node 1 id (src)
       * @param np1 node 1 port (src)
       * @param nt2 node 2 type (dst)
       * @param ni2 node 2 id (dst)
       * @param np2 node 2 port (src)
       */
      swlink(string nt1, datapathid  ni1, uint16_t np1,
	     string nt2, datapathid  ni2, uint16_t np2);

      /** \brief Get json_object representation
       *
       * @return JSON representation
       */
      void get_json(json_object* jo);
    };

    /** \brief Constructor of lavi_swlinks.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    lavi_swlinks(const Context* c, const json_object* node)
      : lavi_links(c, node)
    {}
    
    /** \brief Configure lavi_swlinks.
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
     * @param filters list of filter to check
     */
    virtual void send_list(const Msg_stream& stream, const link_filters& filters);

    /** \brief Start lavi_swlinks.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Function to track link events
     * @param e link event
     * @return CONTINUE always
     */
    Disposition handle_link(const Event& e);

    /** \brief Get instance of lavi_swlinks.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_swlinks*& component);

  private:
    /** \brief Send list of links between switches
     * @param stream socket to send list over
     * @param link_list list of links to send
     * @param add indicate if add or remove
     */
    void send_linklist(const Msg_stream& stream, list<swlink> link_list, 
		       bool add=true);

    /** \brief Check if link matches all filters
     * @param filter filter
     * @param link switch to switch link
     */
    bool match(const link_filters filter, swlink link);

    /** Reference to datapathmem
     */
    datapathmem* dpm;
    /** \brief Reference to topology
     */
    Topology* topo;
  };
}

#endif
