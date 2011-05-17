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

#ifndef QUEUE_CONFIG_IN_HH__
#define QUEUE_CONFIG_IN_HH__

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil 
{
  /** \brief Packet queue property
   *
   * @author ykk
   * @date February 2010
   */
  struct Packet_queue_property
  {
    /** \brief Empty constructor 
     */
    Packet_queue_property()
    { };

  protected:
    /** \brief Set basic header properties
     * @param obj pointer to object's header
     * @param oqph pointer to header of queue property (network order)
     */
    void set_queue_property_header(ofp_queue_prop_header* obj,
				   const ofp_queue_prop_header* oqph);
  };

  /** \brief Packet queue min rate property
   *
   * @author ykk
   * @date February 2010
   */
  struct Packet_queue_min_rate_property:
    public Packet_queue_property, public ofp_queue_prop_min_rate
  {
    /** \brief Constructor
     * @param opqmr pointer to min rate property (network order)
     */
    Packet_queue_min_rate_property(const ofp_queue_prop_min_rate* oqpmr);    
  };

  /** \brief Packet queue config
   *
   * @author ykk
   * @date February 2010
   */
  struct Packet_queue:
    public ofp_packet_queue
  {
    /** \brief Constructor
     * 
     * @param opq pointer to packet queue config in network order
     */
    Packet_queue(const ofp_packet_queue* opq);

    /** Vector of queue property
     */
    std::vector<Packet_queue_property> properties;
  };

  /** \brief Switch reply of queue config
   *
   * @author ykk
   * @date February 2010
   */
  struct Queue_config_in_event
    : public Event,
      public Ofp_msg_event
  {
    /** \brief Constructor
     * 
     * @param dpid reference to switch
     * @param osr pointer to stats reply
     * @param buf reference to buffer
     */
    Queue_config_in_event(const datapathid& dpid, 
			  const ofp_queue_get_config_reply *oqgcr,
			  std::auto_ptr<Buffer> buf);

    /** \brief Empty constructor
     * Only for use within python
     */
    Queue_config_in_event() : Event(static_get_name()) { }

    /** Static name of event
     * @return name of event
     */
    static const Event_name static_get_name() {
      return "Queue_config_in_event";
    }

    /** Datapath id of switch
     */
    datapathid datapath_id;
    /** Port queried
     */
    uint16_t port;
    /** Vector of queue statistics
     */
    std::vector<Packet_queue> queues;

    /** \brief Constructor
     * @param qcie reference to queue config in event
     */
    Queue_config_in_event(const Queue_config_in_event& qcie);
    /** \brief Equal operator
    * @param qcie reference to queue config in event
     */
    Queue_config_in_event& operator=(const Queue_config_in_event& qcie);
  };
};

#endif
