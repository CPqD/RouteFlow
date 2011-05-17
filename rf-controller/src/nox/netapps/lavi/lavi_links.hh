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
#ifndef lavi_links_HH
#define lavi_links_HH

#include "component.hh"
#include "config.h"
#include "messenger/jsonmessenger.hh"

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

  /** \brief lavi_links: base class for all components handle links
   * \ingroup noxcomponents
   * 
   * Provides basic containers for subscribers and
   * function declaration for handling  
   *
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi",
   *   "command" : <string | "request", "subscribe", "unsubscribe">,
   *   "link_type" : <string | "all", ...>,
   * </PRE>
   * with optional parameters
   * <PRE>
   *   "filter" : [ { "node_type" : <string>,
   *                  "node_id" : <string> }, 
   *               ... ]
   * }
   * </PRE>
   * where link will be presented in result if either end matches all
   * of the filter specified.
   *
   * @author ykk
   * @date June 2010
   */
  class lavi_links
    : public Component 
  {
  public:
    /** \brief Structure to hold link filter
     *
     * @author ykk
     * @date June 2010
     */
    struct link_filter
    {
      /** \brief Node type
       */
      string node_type;
      /** \brief Node id
       */
      string node_id;
      
      /** \brief Constructor
       * @param jo JSON object
       */
      link_filter(const json_object& jo);

      /** \brief Compare filter
       * @param other other filter
       */
      bool operator==(const link_filter& other) const;

      /** \brief Check if filter is matched
       * @param nodetype node type
       * @param nodeid node id
       */
      bool match(string nodetype, string nodeid) const;
    };

    /** \brief List of ilters
     */
    typedef list<link_filter> link_filters;

    /** \brief Constructor of lavi_links.
     *
     * @param c context
     * @param node configuration (JSON object)
     */
    lavi_links(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure lavi_links.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start lavi_links.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Add host to JSON dictionary
     * @param jd reference to JSON dictionary
     * @param src source or destination
     * @param host ethernet address of host
     */
    static void json_add_host(json_dict* jd, bool src, const string& host);

    /** \brief Add host to JSON dictionary
     * @param jd reference to JSON dictionary
     * @param src source or destination
     * @param host ethernet address of host
     */
    static void json_add_host(json_dict* jd, bool src, const ethernetaddr host);

    /** \brief Add switch to JSON dictionary
     * @param jd reference to JSON dictionary
     * @param src source or destination
     * @param dpid datapath id of switch
     * @param port port number of switch
     */
    static void json_add_switch(json_dict* jd, bool src, 
			 const string& dpid, const string& port);

    /** \brief Add switch to JSON dictionary
     * @param jd reference to JSON dictionary
     * @param src source or destination
     * @param dpid datapath id of switch
     * @param port port number of switch
     */
    static void json_add_switch(json_dict* jd, bool src, 
			 const datapathid dpid, const uint16_t port);

    /** \brief Function to handle requests
     *
     * @param e json message event
     * @return CONTINUE
     */
    Disposition handle_req(const Event& e);

    /** \brief Function to send list.
     *
     * @param stream message stream to send list to
     * @param filters list of filter to check
     */
    virtual void send_list(const Msg_stream& stream, const link_filters& filters);

    /** \brief Get instance of lavi_links.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_links*& component);

  protected:
    /** \brief String describing node type processed
     */
    string linktype;
    /** \brief List of interested subscribers
     */
    list<pair<Msg_stream*, link_filters> > interested;
    
    /** \brief Compare filters (exact match with order)
     * @param filter1 first filter
     * @param filter2 second filter
     */
    bool compare(const link_filters filter1, link_filters filter2);

    /** \brief Check if node matches all filters
     * @param filter filter
     * @param nodetype1 node 1 type
     * @param nodeid1 node 1 id
     * @param nodetype2 node 2 type
     * @param nodeid2 node 2 id
     */
    bool match(const link_filters filter, 
	       string nodetype1, string nodeid1,
	       string nodetype2, string nodeid2);

    /** \brief Unsubscribe stream
     *
     * @param sock socket to unsubscribe
     * @param filters filters of subscription (exact ordered array), 
     *                else if empty remove all subscription for this socket
     */
    virtual void unsubscribe(Msg_stream* sock, const link_filters& filters);
  };
}

#endif
