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
#ifndef PORT_STATUS_HH
#define PORT_STATUS_HH 1

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"
#include "port.hh"

namespace vigil {

/** \ingroup noxevents
 *
 * Port_status events are thrown for each change in switch port status.
 * Generally this means that a port has been enable or disabled, or
 * a port has been added to a switch.
 *
 */


struct Port_status_event
    : public Event,
      public Ofp_msg_event
{
    Port_status_event(datapathid datapath_id_, uint8_t reason_,
                      const Port& port_)
        : Event(static_get_name()), reason(reason_), port(port_),
          datapath_id(datapath_id_) {}

    Port_status_event(datapathid datapath_id_, const ofp_port_status *ops,
                      std::auto_ptr<Buffer> buf)
        : Event(static_get_name()), Ofp_msg_event(&ops->header, buf),
          reason(ops->reason), port(&ops->desc), datapath_id(datapath_id_)
        {}

    // -- only for use within python
    Port_status_event() : Event(static_get_name()) { ; }

    static const Event_name static_get_name() {
        return "Port_status_event";
    }

    //! Reason for port status change (from ofp_port_reason)
    uint8_t reason;
    Port port;
    datapathid datapath_id;

    Port_status_event(const Port_status_event&);
    Port_status_event& operator=(const Port_status_event&);
};

} // namespace vigil

#endif /* port-status.hh */
