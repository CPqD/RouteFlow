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

#ifndef USER_EVENT_HH
#define USER_EVENT_HH 1

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"

/* User network events */

namespace vigil {

struct User_event {
    enum Reason {
        AUTHENTICATION_EVENT,
        DEAUTHENTICATION_EVENT,
        HARD_TIMEOUT,
        IDLE_TIMEOUT,
        HOST_DELETE,
        USER_DELETE
    };

    static const char *get_reason_string(Reason reason);
};

/** \ingroup noxevents
 *
 * User authentication/deauthentication event.
 *
 * Triggers a user auth/deauth on a host.  On deauthentication, either username
 * or hostname can be wildcarded by setting it to data_cache->get_unknown_id(),
 * however both cannot be wildcarded.
 */

struct User_auth_event
    : public Event
{
    enum Action {
        AUTHENTICATE,
        DEAUTHENTICATE,
    };

    // AUTHENTICATE CONSTRUCTOR
    User_auth_event(int64_t username_, int64_t hostname_,
                    uint32_t idle_timeout_, uint32_t hard_timeout_,
                    User_event::Reason reason_);

    // DEAUTHENTICATE CONSTRUCTOR
    User_auth_event(int64_t username_, int64_t hostname_,
                    User_event::Reason reason_);

    // -- only for use within python
    User_auth_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "User_auth_event";
    }

    Action              action;
    int64_t             username;
    int64_t             hostname;
    uint32_t            idle_timeout;
    uint32_t            hard_timeout;
    User_event::Reason  reason;
    Event               *to_post;
};

/** \ingroup noxevents
 *
 * User join/leave event.
 *
 * Advertises a user as having joined or left a host.
 */

struct User_join_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    User_join_event(Action action_, int64_t username_, int64_t hostname_,
                    User_event::Reason reason_);

    // -- only for use within python
    User_join_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "User_join_event";
    }

    Action              action;
    int64_t             username;
    int64_t             hostname;
    User_event::Reason  reason;
};

} // namespace vigil

#endif /* user_event.hh */
