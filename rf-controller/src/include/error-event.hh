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
#ifndef ERROR_HH
#define ERROR_HH 1

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

struct Error_event
    : public Event,
      public Ofp_msg_event
{
    Error_event(datapathid datapath_id_,
                const ofp_error_msg *oem, std::auto_ptr<Buffer> buffer)
        : Event(static_get_name()),
          Ofp_msg_event(&oem->header, buffer),
          datapath_id(datapath_id_),
          type(ntohs(oem->type)),
          code(ntohs(oem->code))
        { }

    virtual ~Error_event() { }

    static const Event_name static_get_name() {
        return "Error_event";
    }

    datapathid datapath_id;
    uint16_t type;
    uint16_t code;

    Error_event(const Error_event&);
    Error_event& operator=(const Error_event&);
};

} // namespace vigil

#endif /* error-event.hh */
