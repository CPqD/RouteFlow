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
#ifndef FLOW_MOD_EVENT_HH
#define FLOW_MOD_EVENT_HH 1

#include <boost/noncopyable.hpp>
#include "event.hh"
#include "flow-event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

namespace vigil {

/** \ingroup noxevents
 *
 */

struct Flow_mod_event
    : public Event,
      public Ofp_msg_event,
      public Flow_event,
      boost::noncopyable
{
    Flow_mod_event(datapathid datapath_id_, const ofp_flow_mod *fme,
                   std::auto_ptr<Buffer> buf)
        : Event(static_get_name()), Ofp_msg_event(&fme->header, buf),
          datapath_id(datapath_id_) { ; }

    // -- only for use within python
    Flow_mod_event() : Event(static_get_name()) { }

    datapathid datapath_id;

    const ofp_match* get_flow() const {
        return &get_flow_mod()->match;
    }

    const ofp_flow_mod* get_flow_mod() const {
        return reinterpret_cast<const ofp_flow_mod*>(get_ofp_msg());
    }

    static const Event_name static_get_name() {
        return "Flow_mod_event";
    }
};

} // namespace vigil

#endif /* flow-mod-event.hh */
