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
#ifndef ECHO_REQUEST_HH
#define ECHO_REQUEST_HH 1

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

namespace vigil {

struct Echo_request_event
    : public Event,
      public Ofp_msg_event
{
    Echo_request_event(datapathid datapath_id_,
                       const ofp_header *oh, std::auto_ptr<Buffer> buf_)
        : Event(static_get_name()), Ofp_msg_event(oh, buf_),
          datapath_id(datapath_id_)
        {}

    virtual ~Echo_request_event() { }

    const boost::shared_ptr<Buffer>& get_buffer() const { return buf; }

    static const Event_name static_get_name() {
        return "Echo_request_event";
    }

    datapathid datapath_id;

    Echo_request_event(const Echo_request_event&);
    Echo_request_event& operator=(const Echo_request_event&);
};

} // namespace vigil

#endif /* echo-request.hh */
