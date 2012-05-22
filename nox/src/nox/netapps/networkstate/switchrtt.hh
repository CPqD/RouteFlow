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
#ifndef switchrtt_HH
#define switchrtt_HH

#include "component.hh"
#include "config.h"
#include "networkstate/datapathmem.hh"
#include "openflow/openflow.h"
#include <boost/shared_array.hpp>
#include <time.h>

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

/** Default interval to probe switch for RTT
 */
#define SWITCHRTT_DEFAULT_PROBE_INTERVAL 120 //2 mins

namespace vigil
{
  using namespace std;
  using namespace vigil::container;

  /** \brief switchrtt: meaure RTT of switch in network
   * \ingroup noxcomponents
   * 
   * Can configure the interval of probe at runtime using
   * the interval option (giving interval in seconds), e.g.,
   * <PRE>./nox_core switchrtt=interval=600</PRE><BR>
   * set the interval to 10 mins.
   *
   * @author ykk
   * @date December 2009
   */
  class switchrtt
    : public Component 
  {
  public:
    /** \brief Interval to sample each switch (in s)
     */
    time_t interval;
    /** \brief Last recorded RTT of switches
     *
     * Indexed by datapath id in host order.
     */
    hash_map<uint64_t,timeval> rtt;

    /** \brief Constructor of switchrtt.
     *
     * @param c context
     * @param node configuration (JSON object) 
     */
    switchrtt(const Context* c, const json_object* node);
    
    /** \brief Configure switchrtt.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start switchrtt.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Send periodic echo requests
     */
    void periodic_probe();

    /** \brief Handle echo reply.
     * @param e OpenFlow's message event
     * @return CONTINUE
     */
    Disposition handle_openflow_msg(const Event& e);

    /** \brief Handle datapath leave event.
     * 
     * Remove state of departing datapath.
     *
     * @param e datapath leave event
     * @return CONTINUE
     */
    Disposition handle_dp_leave(const Event& e);

    /** \brief Get instance of switchrtt.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    switchrtt*& component);

  private:
    /** \brief Reference to datapath memory
     */
    datapathmem* dpmem;
    /** \brief Map of echo request sent
     *
     * Indexed by datapath id in host order.
     * Tracks xid and send time.
     */
    hash_map<uint64_t,pair<uint32_t,timeval> > echoSent;
    /** \brief Memory for OpenFlow packet
     */
    boost::shared_array<uint8_t> of_raw;
    /** Iterator for probing
     */
    hash_map<uint64_t,Datapath_join_event>::const_iterator dpi;

    /** \brief Get next time to send probe
     *
     * @return time for next probe
     */
    timeval get_next_time();
  };
}

#endif
