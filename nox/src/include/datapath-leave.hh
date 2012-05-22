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
#ifndef DATAPATH_LEAVE_HH
#define DATAPATH_LEAVE_HH 1

#include "event.hh"
#include "netinet++/datapathid.hh"

namespace vigil {

/** \ingroup noxevents
 *
 * Datapath_leave_events are thrown when a switch disconnects from the
 * network.
 *
 */

struct Datapath_leave_event
    : public Event
{
    Datapath_leave_event(datapathid datapath_id_)
        : Event(static_get_name()), datapath_id(datapath_id_) { }

    // -- only for use within python
    Datapath_leave_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Datapath_leave_event";
    }

    //! ID of the departing switch 
    datapathid datapath_id;

private:
    Datapath_leave_event(const Datapath_leave_event&);
    Datapath_leave_event& operator=(const Datapath_leave_event&);
};

} // namespace vigil

#endif /* datapath-leave.hh */
