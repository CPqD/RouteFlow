/* Copyright 2010 (C) Stanford University
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

#ifndef QUEUE_STATS_IN_HH__
#define QUEUE_STATS_IN_HH__

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil 
{
  /** \brief Statistics of a queue
   *
   * @author ykk
   * @date February 2010
   */
  struct Queue_stats
    : public ofp_queue_stats
  {
    /** \brief Constructor
     * @param oqs pointer to stats in network order
     */
    Queue_stats(const ofp_queue_stats* oqs);
  };

  /** \brief Switch reply of queue statistics
   *
   * @author ykk
   * @date February 2010
   */
  struct Queue_stats_in_event
    : public Event,
      public Ofp_msg_event
  {
    /** \brief Constructor
     * 
     * @param dpid reference to switch
     * @param osr pointer to stats reply
     * @param buf reference to buffer
     */
    Queue_stats_in_event(const datapathid& dpid, const ofp_stats_reply *osr,
			 std::auto_ptr<Buffer> buf);

    /** \brief Empty constructor
     * Only for use within python
     */
    Queue_stats_in_event() : Event(static_get_name()) { }

    /** Static name of event
     * @return name of event
     */
    static const Event_name static_get_name() {
      return "Queue_stats_in_event";
    }

    /** Datapath id of switch
     */
    datapathid datapath_id;
    /** Vector of queue statistics
     */
    std::vector<Queue_stats> queues;

    /** \brief Constructor
     * @param qsie reference to queue stats in event
     */
    Queue_stats_in_event(const Queue_stats_in_event& qsie);
    /** \brief Equal operator
    * @param qsie reference to queue stats in event
     */
    Queue_stats_in_event& operator=(const Queue_stats_in_event& qsie);
  };
};

#endif
