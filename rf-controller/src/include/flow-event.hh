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
#ifndef FLOW_EVENT_HH
#define FLOW_EVENT_HH

struct ofp_match;

namespace vigil {

/* Interface class for an event related to a flow. */
class Flow_event
{
public:
    virtual ~Flow_event() { }
    virtual const ofp_match* get_flow() const = 0;
};

} // namespace vigil

#endif /* flow-event.hh */
