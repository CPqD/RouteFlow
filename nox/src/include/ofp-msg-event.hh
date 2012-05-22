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
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef OFP_MSG_EVENT_HH
#define OFP_MSG_EVENT_HH

#include <boost/shared_ptr.hpp>
#include "buffer.hh"
#include "openflow/openflow.h"

namespace vigil
{

/* XXX Every openflow message relates to a datapath, so should there be a
 * datapath ID in here? */
class Ofp_msg_event
{
public:
    virtual ~Ofp_msg_event() { }
    Ofp_msg_event(const ofp_header* ofp_msg_, std::auto_ptr<Buffer> buf_)
        : buf(buf_.release()), ofp_msg(ofp_msg_) { }
    Ofp_msg_event(const ofp_header* ofp_msg_, boost::shared_ptr<Buffer> buf_)
        : buf(buf_), ofp_msg(ofp_msg_) { }
    Ofp_msg_event()
        : ofp_msg((ofp_header*)NULL) { }
    virtual const ofp_header *get_ofp_msg() const { return ofp_msg; }

    boost::shared_ptr<Buffer> buf;

    uint8_t version() const { return ofp_msg->version; }
    uint8_t type() const { return ofp_msg->type; }
    uint16_t length() const { return ntohs(ofp_msg->length); }
    uint32_t xid() const { return ofp_msg->xid; }

private:
    const struct ofp_header *ofp_msg;
};

} // namespace vigil

#endif  // -- OFP_MSG_EVENT_HH
