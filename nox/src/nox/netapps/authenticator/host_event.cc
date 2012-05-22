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

#include "host_event.hh"

namespace vigil {

const char*
Host_event::get_reason_string(Host_event::Reason reason)
{
    static const char* auth_event = "authentication event";
    static const char* auto_auth = "auto authentication";
    static const char* auto_nwaddr_add = "auto nwaddr add";
    static const char* deauth_event = "deauthentication event";
    static const char* auto_nwaddr_rm = "auto nwaddr remove";
    static const char* internal_loc = "internal location";
    static const char* binding_change = "binding change";
    static const char* hard_timeout = "hard timeout";
    static const char* idle_timeout = "idle timeout";
    static const char* switch_leave = "switch leave";
    static const char* location_leave = "location leave";
    static const char* host_delete = "host delete";

    switch (reason) {
    case AUTHENTICATION_EVENT:
        return auth_event;
    case AUTO_AUTHENTICATION:
        return auto_auth;
    case NWADDR_AUTO_ADD:
        return auto_nwaddr_add;
    case DEAUTHENTICATION_EVENT:
        return deauth_event;
    case NWADDR_AUTO_REMOVE:
        return auto_nwaddr_rm;
    case INTERNAL_LOCATION:
        return internal_loc;
    case BINDING_CHANGE:
        return binding_change;
    case HARD_TIMEOUT:
        return hard_timeout;
    case IDLE_TIMEOUT:
        return idle_timeout;
    case SWITCH_LEAVE:
        return switch_leave;
    case LOCATION_LEAVE:
        return location_leave;
    case HOST_DELETE:
        return host_delete;
    default:
        break;
    }
    return "";
}

// AUTHENTICATE constructor
Host_auth_event::Host_auth_event(datapathid datapath_id_, uint16_t port_,
                                 ethernetaddr dladdr_, uint32_t nwaddr_,
                                 int64_t hostname_, int64_t host_netid_,
                                 uint32_t idle_timeout_, uint32_t hard_timeout_,
                                 Host_event::Reason reason_)
    : Event(static_get_name()), action(AUTHENTICATE), datapath_id(datapath_id_),
      port(port_), dladdr(dladdr_), nwaddr(nwaddr_), hostname(hostname_),
      host_netid(host_netid_), idle_timeout(idle_timeout_),
      hard_timeout(hard_timeout_), reason(reason_), to_post(NULL)
{}

// DEAUTHENTICATE constructor
Host_auth_event::Host_auth_event(datapathid datapath_id_, uint16_t port_,
                                 ethernetaddr dladdr_, uint32_t nwaddr_,
                                 int64_t hostname_, int64_t host_netid_,
                                 uint32_t enabled_fields_, Host_event::Reason reason_)
    : Event(static_get_name()), action(DEAUTHENTICATE),
      datapath_id(datapath_id_), port(port_), dladdr(dladdr_), nwaddr(nwaddr_),
      hostname(hostname_), host_netid(host_netid_),
      enabled_fields(enabled_fields_), reason(reason_), to_post(NULL)
{}

Host_bind_event::Host_bind_event(Action action_, datapathid datapath_id_,
                                 uint16_t port_, int64_t switchname_,
                                 int64_t locname_, ethernetaddr dladdr_,
                                 int64_t hostname_, int64_t host_netid_,
                                 Host_event::Reason reason_)
    : Event(static_get_name()), action(action_), datapath_id(datapath_id_),
      port(port_), switchname(switchname_), locname(locname_),
      dladdr(dladdr_), nwaddr(0), hostname(hostname_),
      host_netid(host_netid_), reason(reason_)
{}

Host_bind_event::Host_bind_event(Action action_, ethernetaddr dladdr_,
                                 uint32_t nwaddr_, int64_t hostname_,
                                 int64_t host_netid_,
                                 Host_event::Reason reason_)
    : Event(static_get_name()), action(action_),
      datapath_id(datapathid::from_host(0)), port(0), switchname(0),
      locname(0), dladdr(dladdr_),
      nwaddr(nwaddr_), hostname(hostname_), host_netid(host_netid_),
      reason(reason_)
{}

Host_join_event::Host_join_event(Action action_, int64_t hostname_,
                                 Host_event::Reason reason_)
    : Event(static_get_name()), action(action_), hostname(hostname_),
      reason(reason_)
{}

}
