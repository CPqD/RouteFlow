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

#include "user_event.hh"

namespace vigil {

const char*
User_event::get_reason_string(User_event::Reason reason)
{
    static const char* auth_event = "authentication event";
    static const char* deauth_event = "deauthentication event";
    static const char* hard_timeout = "hard timeout";
    static const char* idle_timeout = "idle timeout";
    static const char* host_delete = "host delete";
    static const char* user_delete = "user delete";

    switch (reason) {
    case AUTHENTICATION_EVENT:
        return auth_event;
    case DEAUTHENTICATION_EVENT:
        return deauth_event;
    case HARD_TIMEOUT:
        return hard_timeout;
    case IDLE_TIMEOUT:
        return idle_timeout;
    case HOST_DELETE:
        return host_delete;
    case USER_DELETE:
        return user_delete;
    default:
        break;
    }
    return "";
}

User_auth_event::User_auth_event(int64_t username_, int64_t hostname_,
                                 uint32_t idle_timeout_, uint32_t hard_timeout_,
                                 User_event::Reason reason_)
    : Event(static_get_name()), action(AUTHENTICATE), username(username_),
      hostname(hostname_), idle_timeout(idle_timeout_),
      hard_timeout(hard_timeout_), reason(reason_), to_post(NULL)
{}

User_auth_event::User_auth_event(int64_t username_, int64_t hostname_,
                                 User_event::Reason reason_)
    : Event(static_get_name()), action(DEAUTHENTICATE), username(username_),
      hostname(hostname_), idle_timeout(0), hard_timeout(0), reason(reason_),
      to_post(NULL)
{}

User_join_event::User_join_event(Action action_, int64_t username_,
                                 int64_t hostname_, User_event::Reason reason_)
    : Event(static_get_name()), action(action_), username(username_),
      hostname(hostname_), reason(reason_)
{}

}
