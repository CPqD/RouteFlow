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
#ifndef OPENFLOW_EVENT_HH
#define OPENFLOW_EVENT_HH 1

#include <memory>
#include "openflow.hh"

namespace vigil {

class Buffer;
class Event;

/** \brief Convert OpenFlow packets into specific events.
 *
 * @param oconn OpenFlow connection
 * @param p buffer with message
 * @return event
 */
Event* openflow_packet_to_event(boost::shared_ptr<Openflow_connection>
        oconn, std::auto_ptr<Buffer> p);

/** \brief Convert OpenFlow packets into Openflow_msg_event.
 *
 * @param oconn OpenFlow connection
 * @param p buffer with message
 * @return Openflow_msg_event
 */
Event* openflow_msg_to_event(boost::shared_ptr<Openflow_connection>
        oconn, std::auto_ptr<Buffer> p);

} // namespace vigil

#endif /* openflow-event.hh */
