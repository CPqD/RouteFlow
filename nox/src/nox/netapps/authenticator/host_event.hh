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

#ifndef HOST_EVENT_HH
#define HOST_EVENT_HH 1

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"

/* Host network events */

namespace vigil {

/* Possible reasons for various host events. */
/* SWIG didn't like namespace, so using struct. */

struct Host_event {
    enum Reason {
        AUTHENTICATION_EVENT,     // Add reasons
        AUTO_AUTHENTICATION,
        NWADDR_AUTO_ADD,
        DEAUTHENTICATION_EVENT,   // Remove reasons
        NWADDR_AUTO_REMOVE,       // want this?
        INTERNAL_LOCATION,
        BINDING_CHANGE,
        HARD_TIMEOUT,
        IDLE_TIMEOUT,
        SWITCH_LEAVE,
        LOCATION_LEAVE,
        HOST_DELETE,
        HOST_NETID_DELETE
    };

    static const char *get_reason_string(Reason reason);
};

/** \ingroup noxevents
 *
 * Host authentication/deauthentication event.
 *
 * Triggers a host auth/deauth at a location for a set of addresses.  On
 * authentication, if nwaddr is not set to zero and the mac address is not that
 * of a router, the host will automatically be authenticated for nwaddr == 0 at
 * the location.
 *
 * On deauthentication, fields can be wildcarded to deauthenticate a set of
 * bindings.  Bindings matching the non-wildcarded fields will be
 * deauthenticated.  These could be nwaddr bindings for a dladdr, an entire
 * dladdr, or a location on a particular dladdr.  These bindings can be
 * described with their actual values, or by using hostname.  If both nwaddr
 * and location values are specified by the description (either directly as
 * non-wildcarded, or indirecty through hostname), the location is removed if
 * the nwaddr host owns the dladdr, allowing the nwaddr to remain authenticated
 * for other locations on the dladdr, otherwise the nwaddr binding is removed,
 * leaving the dladdr's host authenticated for the location.
 *
 * All integer values are stored in host byte order, and should be passed in as
 * such.
 */

struct Host_auth_event
    : public Event
{
    enum Action {
        AUTHENTICATE,
        DEAUTHENTICATE,
    };

    enum Enabled_field {
        EF_SWITCH        = 0x1 << 0,
        EF_LOCATION      = 0x1 << 1,
        EF_DLADDR        = 0x1 << 2,
        EF_NWADDR        = 0x1 << 3,
        EF_HOSTNAME      = 0x1 << 4,
        EF_HOST_NETID  = 0x1 << 5,
        EF_ALL           = (0x1 << 6) - 1
    };

    // AUTHENTICATE constructor
    Host_auth_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t idle_timeout_,
                    uint32_t hard_timeout_, Host_event::Reason reason_);

    // DEAUTHENTICATE constructor
    Host_auth_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t enabled_fields_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_auth_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Host_auth_event";
    }

    Action              action;
    datapathid          datapath_id;
    uint16_t            port;
    ethernetaddr        dladdr;
    uint32_t            nwaddr;         // set to zero if no IP to auth
    int64_t             hostname;
    int64_t             host_netid;
    uint32_t            idle_timeout;
    uint32_t            hard_timeout;
    uint32_t            enabled_fields; // bit_mask of fields to observe
    Host_event::Reason  reason;
    Event               *to_post;       // event to post on auth completion
};

/** \ingroup noxevents
 * Host binding add/delete event.
 *
 * Advertises a binding as having been added/removed from a host's active set
 * of bindings. Posted for each mac address authenticated for a location, and
 * each network address authenticated for a mac address.  Includes the netid
 * owning the binding.  If both the location and nwaddr are set to zero, then
 * signals a mac address binding.
 *
 * All integer values are stored in host byte order, and should be passed in as
 * such.
 */

struct Host_bind_event
    : public Event
{
    enum Action {
        ADD,
        REMOVE
    };

    // Location binding constructor
    Host_bind_event(Action action_, datapathid datapath_id_, uint16_t port_,
                    int64_t switchname_, int64_t locname_,
                    ethernetaddr dladdr_, int64_t hostname_, int64_t host_netid_,
                    Host_event::Reason reason_);

    // Nwaddr binding constructor
    Host_bind_event(Action action_, ethernetaddr dladdr_, uint32_t nwaddr_,
                    int64_t hostname_, int64_t host_netid_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_bind_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Host_bind_event";
    }

    Action              action;
    datapathid          datapath_id;  // set to zero if dladdr or nwaddr binding
    uint16_t            port;
    int64_t             switchname;
    int64_t             locname;
    ethernetaddr        dladdr;       // should never be 0!
    uint32_t            nwaddr;       // set to zero if loc or dladdr binding
    int64_t             hostname;
    int64_t             host_netid;
    Host_event::Reason  reason;
};

/** \ingroup noxevents
 * Host join/leave event.
 *
 * Advertises a host as having joined or left the network.  A host has 'joined'
 * the network if when it authenticates for a new set of bindings, no active
 * bindings for the host already exist.  Thus join events are not posted for
 * bindings the host authenticates with on top of its first set (See
 * Host_bind_event for this information.).
 *
 * A host 'leaves' the network when its last bindings are deauthenticated and
 * it no longer has any active bindings.
 *
 * All integer values are stored in host byte order, and should be passed in as
 * such.
 */

struct Host_join_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    Host_join_event(Action action_, int64_t hostname_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_join_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Host_join_event";
    }

    Action              action;
    int64_t             hostname;
    Host_event::Reason  reason;
};

} // namespace vigil

#endif /* host_event.hh */
