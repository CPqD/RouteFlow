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
#ifndef lavi_flows_HH
#define lavi_flows_HH

#include "component.hh"
#include "config.h"
#include "messenger/jsonmessenger.hh"
#include "flow.hh"

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

  /** \brief lavi_flows: base class for components handle flows
   * \ingroup noxcomponents
   * 
   * Provides basic containers for subscribers and
   * function declaration for handling  
   *
   * A request should be of the following JSON form
   * <PRE>
   * {
   *   "type" : "lavi"
   *   "command" : <string | "request", "subscribe", "unsubscribe">
   *   "flow_type" : <string | "all", ...>
   * }
   * </PRE>
   *
   * @author ykk
   * @date June 2010
   */
  class lavi_flows
    : public Component 
  {
  public:
    /** \brief Retrieve stored flows?
     */
    bool show_ongoing;

    /** \brief Constructor of lavi_flows.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    lavi_flows(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure lavi_flows.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start lavi_flows.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Function to handle requests
     *
     * @param e json message event
     * @return CONTINUE
     */
    Disposition handle_req(const Event& e);

    /** \brief Function to send list.
     *
     * @param stream message stream to send list to
     */
    virtual void send_list(const Msg_stream& stream);

    /** \brief Get instance of lavi_flows.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    lavi_flows*& component);

  protected:
    /** \brief String describing flow type processed
     */
    string flowtype;
    /** \brief List of interested subscribers
     */
    list<Msg_stream*> interested;
    
    /** \brief Unsubscribe stream
     *
     * @param sock socket to unsubscribe
     */
    virtual void unsubscribe(Msg_stream* sock);

    /** \brief Return flow id
     *
     * @param flow reference to flow
     * @return cookie, or hash code (if cookie==0)
     */
    uint64_t get_id(const Flow& flow);
  };
}

#endif
