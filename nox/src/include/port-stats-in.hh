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

#ifndef PORT_STATS_IN_HH__
#define PORT_STATS_IN_HH__

#include <string>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil {



struct Port_stats
{
    Port_stats(uint16_t pn) : 
        port_no(pn), 
        rx_packets(0), tx_packets(0), rx_bytes(0), tx_bytes(0),
        rx_dropped(0), tx_dropped(0), rx_errors(0), tx_errors(0),
        rx_frame_err(0), rx_over_err(0), rx_crc_err(0), collisions(0) { ; }

    Port_stats(const Port_stats& ps) :
        port_no(ps.port_no),
	rx_packets(ps.rx_packets), 
        tx_packets(ps.tx_packets), 
        rx_bytes(ps.rx_bytes), 
        tx_bytes(ps.tx_bytes), 
        rx_dropped(ps.rx_dropped), 
        tx_dropped(ps.tx_dropped), 
        rx_errors(ps.rx_errors), 
        tx_errors(ps.tx_errors), 
        rx_frame_err(ps.rx_frame_err), 
        rx_over_err(ps.rx_over_err), 
        rx_crc_err(ps.rx_crc_err), 
        collisions(ps.collisions) { ; }

    Port_stats(struct ofp_port_stats *ops) :
        port_no(ntohs(ops->port_no)), 
        rx_packets(ntohll(ops->rx_packets)), 
        tx_packets(ntohll(ops->tx_packets)), 
        rx_bytes(ntohll(ops->rx_bytes)), 
        tx_bytes(ntohll(ops->tx_bytes)), 
        rx_dropped(ntohll(ops->rx_dropped)), 
        tx_dropped(ntohll(ops->tx_dropped)), 
        rx_errors(ntohll(ops->rx_errors)), 
        tx_errors(ntohll(ops->tx_errors)), 
        rx_frame_err(ntohll(ops->rx_frame_err)), 
        rx_over_err(ntohll(ops->rx_over_err)), 
        rx_crc_err(ntohll(ops->rx_crc_err)), 
        collisions(ntohll(ops->collisions)) { ; }

    uint16_t port_no;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_dropped;
    uint64_t tx_dropped;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t rx_frame_err;
    uint64_t rx_over_err;
    uint64_t rx_crc_err;
    uint64_t collisions;

    Port_stats& operator=(const Port_stats& ps)
    {
        port_no = ps.port_no;
	rx_packets = ps.rx_packets;
	tx_packets = ps.tx_packets;
	rx_bytes = ps.rx_bytes;
	tx_bytes = ps.tx_bytes;
	rx_dropped = ps.rx_dropped;
	tx_dropped = ps.tx_dropped;
	rx_errors = ps.rx_errors;
	tx_errors = ps.tx_errors;
	rx_frame_err = ps.rx_frame_err;
	rx_over_err = ps.rx_over_err;
	rx_crc_err = ps.rx_crc_err;
	collisions = ps.collisions;

	return *this;
    }
};

/** \ingroup noxevents
 *
 * Port_stats events are thrown for each port stats message received
 * from the switches.  Port stats messages are sent in response to
 * OpenFlow port stats requests.  
 *
 * Each messages contains the statistics for all ports on a switch.
 * The available values are contained in the Port_stats struct.
 *
 */

struct Port_stats_in_event
    : public Event,
      public Ofp_msg_event
{
    Port_stats_in_event(const datapathid& dpid, const ofp_stats_reply *osr,
                        std::auto_ptr<Buffer> buf);

    // -- only for use within python
    Port_stats_in_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Port_stats_in_event";
    }

    //! ID of switch sending the port stats message 
    datapathid datapath_id;
    //! List of port statistics 
    std::vector<Port_stats> ports;

    Port_stats_in_event(const Port_stats_in_event&);
    Port_stats_in_event& operator=(const Port_stats_in_event&);

    void add_port(uint16_t pn);
    void add_port(struct ofp_port_stats *ops);

}; // Port_stats_in_event

inline
Port_stats_in_event::Port_stats_in_event(const datapathid& dpid,
                                         const ofp_stats_reply *osr,
                                         std::auto_ptr<Buffer> buf)
    : Event(static_get_name()), Ofp_msg_event(&osr->header, buf)
{
    datapath_id  = dpid;
}

inline
void
Port_stats_in_event::add_port(uint16_t pn)
{
    ports.push_back(Port_stats(pn));
}

inline
void
Port_stats_in_event::add_port(struct ofp_port_stats *ops)
{
    ports.push_back(Port_stats(ops));
}

} // namespace vigil

#endif // PORT_STATS_IN_HH__
