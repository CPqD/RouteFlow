/* Copyright 2009 (C) Stanford University.
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
#ifndef datapathmem_HH
#define datapathmem_HH

#include "component.hh"
#include "config.h"
#include "hash_map.hh"
#include "datapath-join.hh"

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

  /** \brief datapathmem: Datapath memory to memorize datapath information
   * \ingroup noxcomponents
   * 
   * Also join events of the datapath. 
   *
   * @author ykk
   * @date December 2009
   */
  class datapathmem
    : public Component 
  {
  public:
    /** \brief Constructor of datapathmem.
     *
     * @param c context
     * @param node configuration (JSON object) 
     */
    datapathmem(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Destructor of datapathmem.
     *
     * Clear memory.  Forget!
     */
    ~datapathmem();

    /** \brief Configure datapathmem.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start datapathmem.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of datapathmem.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    datapathmem*& component);

    /** \brief Handle datapath join event.
     * 
     * Add to dp_events.
     *
     * @param e datapath join event
     * @return CONTINUE
     */
    Disposition handle_dp_join(const Event& e);

    /** \brief Handle datapath leave event.
     * 
     * Remove from dp_events.
     *
     * @param e datapath leave event
     * @return CONTINUE
     */
    Disposition handle_dp_leave(const Event& e);

    /** \brief Handle port status changes
     *
     * @param e port status event
     * @return CONTINUE
     */
    Disposition handle_port_event(const Event& e);

    /** \brief Get link bandwidth
     * 
     * @param dpid datapath id of switch
     * @param port port number
     * @return speed in mbps (0 if port is not found)
     */
    uint32_t get_link_speed(datapathid dpid, uint16_t port);
    
    /** \brief Hash map of datapath join event.
     *
     * Indexed by datapath id in host order.
     * This hash_map holds the join event that will be sent
     * considering all the Port_status_event changes.
     */
    hash_map<uint64_t,Datapath_join_event> dp_events;

  private:
  };
}

#endif
