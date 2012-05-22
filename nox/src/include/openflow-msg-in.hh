/* Copyright 2008 (C) Stanford University.
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

#ifndef OPENFLOW_MSG_HH__
#define OPENFLOW_MSG_HH__

#include <string>
#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"
#include "openflow/openflow.h"

namespace vigil 
{

  /** \ingroup noxevents
   * \brief OpenFlow message event
   *
   * Allows all OpenFlow messages to be exposed to components
   * as events.  This is useful for vendor extensions and so on.
   *
   * @author ykk
   * @date 2008
   */
  struct Openflow_msg_event
    : public Event,
      public Ofp_msg_event
  {
    /** \brief Constructor
     * 
     * @param dpid datapath associated with message
     * @param of_msg_ header pointer to message
     * @param buf buffer containing message
     */
    Openflow_msg_event(const datapathid& dpid, const ofp_header* ofp_msg_,
		       std::auto_ptr<Buffer> buf);

    /** \brief Empty constructor.
     *
     *  Only for use within python
     */
    Openflow_msg_event() : Event(static_get_name()) { }

    /** \brief Return static name for event.
     *
     * @return unique string naming event
     */
    static const Event_name static_get_name() {
        return "Openflow_msg_event";
    }

    /** Datapath message is from.
     */
    datapathid datapath_id;
}; 

inline
Openflow_msg_event::Openflow_msg_event(const datapathid& dpid, const ofp_header* ofp_msg_,
				       std::auto_ptr<Buffer> buf)
  : Event(static_get_name()), Ofp_msg_event(ofp_msg_, buf)
{
    datapath_id = dpid;
}

} // namespace vigil
#endif
