/* Copyright 2008 (C) Nicira, Inc.
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
#ifndef PACKET_IN_HH
#define PACKET_IN_HH 1

#include <memory>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <arpa/inet.h>
#include "openflow/openflow.h"
#include "buffer.hh"
#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"
#include "openflow.hh"
#include "flow.hh"

namespace vigil {

/** \ingroup noxevents
 *
 * Packet in events are thrown for each packet received by nox. The
 * packet buffer is stored in the parent class, Ofp_msg_event. To
 * access the buffer, use the get_buffer() member function. 
 *
 * Unless a component retains a shared_ptr to the buffer, packets 
 * are discarded after the Packet_in_event is thrown.  Sending 
 * the packet back onto the network must be done explicitly.
 *
 */

struct Packet_in_event
    : public Event,
      public Ofp_msg_event
{
    Packet_in_event(datapathid datapath_id_, uint16_t in_port_,
                    std::auto_ptr<Buffer> buf_, size_t total_len_,
                    uint32_t buffer_id_, uint8_t reason_)
        : Event(static_get_name()), Ofp_msg_event((ofp_header*) NULL, buf_),
          datapath_id(datapath_id_), in_port(in_port_), total_len(total_len_),
          buffer_id(buffer_id_), reason(reason_), flow(htons(in_port), *buf)
        { }

    Packet_in_event(datapathid datapath_id_, uint16_t in_port_,
                    boost::shared_ptr<Buffer> buf_, size_t total_len_,
                    uint32_t buffer_id_, uint8_t reason_)
        : Event(static_get_name()), Ofp_msg_event((ofp_header*) NULL, buf_),
          datapath_id(datapath_id_), in_port(in_port_), total_len(total_len_),
          buffer_id(buffer_id_), reason(reason_), flow(htons(in_port), *buf)
        { }

    Packet_in_event(datapathid datapath_id_,
                    const ofp_packet_in *opi, std::auto_ptr<Buffer> buf_)
        : Event(static_get_name()), Ofp_msg_event(&opi->header, buf_),
          datapath_id(datapath_id_),
          in_port(ntohs(opi->in_port)),
          total_len(ntohs(opi->total_len)),
          buffer_id(ntohl(opi->buffer_id)),
          reason(opi->reason), flow(htons(in_port), *buf)
        { }

    virtual ~Packet_in_event() { }

    const boost::shared_ptr<Buffer>& get_buffer() const { return buf; }

    static const Event_name static_get_name() {
        return "Packet_in_event";
    }

    //! ID of switch the packet was received from 
    datapathid datapath_id;
    //! Switch specific port number packet was received on 
    uint16_t in_port;
    //! The total length of the packet including all headers 
    size_t   total_len;
    uint32_t buffer_id;
    //! Did packet match an entry, or not? 
    /*!
      One of ofp_packet_in_reason values describing why the packet was
      sent to the controller.  Possible values include:
      <ul>
        <li> OFPR_NO_MATCH : There was no matching entry
        <li> OFPR_ACTION : There was a match and the action was "send to controller"
      </ul>
     */
    uint8_t  reason;

    /** \brief Flow interpretation
     */
    Flow flow;

    Packet_in_event(const Packet_in_event&);
    Packet_in_event& operator=(const Packet_in_event&);
};

} // namespace vigil

#endif /* packet-in.hh */
