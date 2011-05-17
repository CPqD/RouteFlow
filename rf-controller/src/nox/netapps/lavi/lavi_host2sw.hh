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
#ifndef lavi_host2sw_HH
#define lavi_host2sw_HH

#include "component.hh"
#include "config.h"
#include "hoststate/hosttracker.hh"
#include "lavi_links.hh"

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

  /** \brief lavi_host2sw: Expose links between host and switch (host to switch)
   * \ingroup noxcomponents
   * 
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi",
   *   "command" : <string | "request", "subscribe", "unsubscribe">,
   *   "link_type" : <string | "all", "host2sw">,
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
   *   "link_type" : "host2sw"
   *   "links" : [ { "src type" : "host",
   *                 "src id" : <string>,
   *                 "dst type" : "switch",
   *                 "dst id" : <string>,
   *                 "dst port" : <string>
   *               }, ...]
   * }
   * </PRE>
   *
   * @see lavi_links
   * @see hosttracker
   * @author ykk
   * @date July 2010
   */
  class lavi_host2sw
    : public lavi_links 
  {
  public:
    /** \brief Constructor of lavi_host2sw.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    lavi_host2sw(const Context* c, const json_object* node)
      : lavi_links(c, node)
    {}
    
    /** \brief Configure lavi_host2sw.
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

    /** \brief Start lavi_host2sw.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Function to track link events
     * @param e link event
     * @return CONTINUE always
     */
    Disposition handle_hostloc(const Event& e);

    /** \brief Get instance of lavi_host2sw.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_host2sw*& component);

  private:
    /** \brief Send  host attached to switch
     * @param stream socket to send list over
     * @param host host
     * @param loc location/link to send
     * @param add indicate if add or remove
     */
    void send_link(const Msg_stream& stream, 
		   ethernetaddr host,
		   const hosttracker::location loc, 
		   bool add=true);

    /** \brief Check if host to switch link matches all filters
     * @param filter filter
     * @param host host
     * @param loc location/host to switch link
     */
    bool match(const link_filters filter, ethernetaddr host, 
	       hosttracker::location loc);

    /* \brief Get JSON representation of host to switch link
     * @param jo JSON object to populate
     * @param host host
     * @param loc location/host to switch link
     */
    void get_json(json_object* jo, const ethernetaddr host,
		  const hosttracker::location link);


    /** \brief Reference to hosttracker
     */
    hosttracker* ht;
  };
}

#endif
