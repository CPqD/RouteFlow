/* Copyright 2008, 2009 (C) Nicira, Inc.
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

#ifndef PRINCIPAL_EVENT_HH
#define PRINCIPAL_EVENT_HH 1

#include <boost/noncopyable.hpp>
#include <string>

#include "event.hh"
#include "datatypes.hh"

/*
 * Principal events.
 */

namespace vigil {

/*
 * Principal delete event.
 */

struct Principal_delete_event
    : public Event,
      boost::noncopyable
{
    Principal_delete_event(PrincipalType type_,
                           int64_t id_);

    // -- only for use within python
    Principal_delete_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Principal_delete_event";
    }

    PrincipalType type;
    int64_t id;
};

} // namespace vigil

#endif /* principal_event.hh */
