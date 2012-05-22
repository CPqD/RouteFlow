/* Copyright 2009 (C) Nicira, Inc.
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

#ifndef SWITCH_EVENT_HH
#define SWITCH_EVENT_HH 1

#include "event.hh"
#include "netinet++/datapathid.hh"

/* DP join with switch id */

namespace vigil {

/** \ingroup noxevents
 *
 * Switch join/leave event.
 *
 * Notifies listeners of a datapath join or leave with the name binding
 * assigned to that datapath.
 *
 * All integer values are stored in host byte order, and should be passed in as
 * such.
 */

struct Switch_bind_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    Switch_bind_event(Action action_, const datapathid& dp,
                      int64_t switchname_);

    // -- only for use within python
    Switch_bind_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Switch_bind_event";
    }

    Action              action;
    datapathid          datapath_id;
    int64_t             switchname;
};

} // namespace vigil

#endif /* switch_event.hh */
