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
#ifndef linkload_HH
#define linkload_HH

#include "component.hh"
#include "config.h"
#include "hash_map.hh"
#include "port-stats-in.hh"
#include "network_graph.hh"
#include "netinet++/datapathid.hh"
#include "datapathmem.hh"
#include <boost/shared_array.hpp>

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/**  Default interval to query for link load (in s)
 */
#define LINKLOAD_DEFAULT_INTERVAL 60

namespace vigil
{
  using namespace std;
  using namespace vigil::container;
  /** \brief linkload: tracks port stats of links and its load
   * \ingroup noxcomponents
   * 
   * Refers to links using its source datapath id and port.
   *
   * You can configure the interval of query using keyword
   * interval, e.g.,
   * ./nox_core -i ptcp:6633 linkload=interval=10
   *
   * @author ykk
   * @date February 2010
   */
  class linkload
    : public Component 
  {
  public:
    /** Definition switch and port
     */
    typedef vigil::network::switch_port switchport;

    /** Hash map of switch/port to stats
     */
    hash_map<switchport, Port_stats> statmap;

    /** Load of switch/port (in bytes)
     */
    struct load
    {
      /** Transmission load (in bytes)
       */
      uint64_t txLoad;
      /** Reception load (in bytes)
       */
      uint64_t rxLoad;
      /** Interval
       */
      time_t interval;

      load(uint64_t txLoad_, uint64_t rxLoad_, time_t interval_):
	txLoad(txLoad_), rxLoad(rxLoad_), interval(interval_)
      {}
    };

    /** Hash map of switch/port to load
     */
    hash_map<switchport, load> loadmap;

    /** Interval to query for load
     */
    time_t load_interval;

    /** \brief Constructor of linkload.
     *
     * @param c context
     * @param node configuration (JSON object) 
     */
    linkload(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Configure linkload.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Periodic port stat probe function
     */
    void stat_probe();

    /** Get link load ratio.
     * @param dpid datapath id of switch
     * @param port port number
     * @param tx transmit load
     * @return ratio of link bandwidth used (-1 if invalid result)
     */
    float get_link_load_ratio(datapathid dpid, uint16_t port, bool tx=true);

    /** Get link load.
     * @param dpid datapath id of switch
     * @param port port number
     * @return ratio of link bandwidth used (interval = 0 for invalid result)
     */
    load get_link_load(datapathid dpid, uint16_t port);

    /** \brief Handle port stats in
     * @param e port stats in event
     * @return CONTINUE
     */
    Disposition handle_port_stats(const Event& e);

    /** \brief Handle datapath leave (remove ports)
     * @param e datapath leave event
     * @return CONTINUE
     */
    Disposition handle_dp_leave(const Event& e);

    /** \brief Handle port status change
     * @param e port status event
     * @return CONTINUE
     */
    Disposition handle_port_event(const Event& e);

    /** \brief Start linkload.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of linkload.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    linkload*& component);

  private:
    /** \brief Reference to datapath memory
     */
    datapathmem* dpmem;
    /** Iterator for probing
     */
    hash_map<uint64_t,Datapath_join_event>::const_iterator dpi;

    /** \brief Send port stats request for switch and port
     * @param dpid switch to send port stats request to
     * @param port port to request stats for
     */
    void send_stat_req(const datapathid& dpid, uint16_t port=OFPP_ALL);

    /** \brief Get next time to send probe
     *
     * @return time for next probe
     */
    timeval get_next_time();
    /** \brief Memory for OpenFlow packet
     */
    boost::shared_array<uint8_t> of_raw;
    /** \brief Update load map
     * @param dpid switch to send port stats request to
     * @param port port to request stats for
     * @param oldstat old port stats
     * @param newstat new port stats
     */
    void updateLoad(const datapathid& dpid, uint16_t port,
		    const Port_stats& oldstat,
		    const Port_stats& newstat);
		    
  };
}

#endif
