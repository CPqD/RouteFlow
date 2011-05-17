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
%{
#include "event.hh"
#include "pyevent.hh"

using namespace vigil;
%}

%include "std_string.i"
%include "cstring.i"

enum Disposition { CONTINUE, STOP };

/*
 * An Event represents a low-level or high-level event in the network.  The
 * Event class exists almost solely to provide a convenient way to pass any
 * kind of event around.  All interesting data exists in Event's derived
 * classes.
 */

typedef std::string Event_name;

class Event {
public:
    virtual ~Event() { }
    std::string get_class_name();
protected:
    Event(Event_name name);
private:
    Event_name name;
};

class pyevent : public Event 
{
public:
    pyevent(Event_name);
    pyevent(Event_name, PyObject* pyarg);
    pyevent();
};

/* Trimmed core event declarations */

struct Datapath_join_event
    : public Event
{
    static const std::string static_get_name();

private:
    Datapath_join_event();
};


struct Datapath_leave_event
    : public Event
{
    static const std::string static_get_name();

private:
    Datapath_leave_event();
};

struct Switch_mgr_join_event
    : public Event
{
    static const std::string static_get_name();

private:
    Switch_mgr_join_event();
};

struct Switch_mgr_leave_event
    : public Event
{
    static const std::string static_get_name();

private:
    Switch_mgr_leave_event();
};

struct Flow_removed_event
    : public Event
{
    static const std::string static_get_name();

private:
    Flow_removed_event();
};

struct Flow_mod_event
    : public Event
{
    static const std::string static_get_name();

private:
    Flow_mod_event();
};

struct Packet_in_event
    : public Event
{
    static const std::string static_get_name();

private:
    Packet_in_event();
};

struct Echo_request_event
    : public Event
{
    static const std::string static_get_name();

private:
    Echo_request_event();
};

struct Port_status_event
    : public Event
{
    static const std::string static_get_name();

private:
    Port_status_event();
};

struct Bootstrap_complete_event
    : public Event
{
    static const std::string static_get_name();

private:
    Bootstrap_complete_event();
};

struct Table_stats_in_event
    : public Event
{
    static const std::string static_get_name();

private:
    Table_stats_in_event();
};

struct Port_stats_in_event
    : public Event
{
    static const std::string static_get_name();

private:
    Port_stats_in_event();
};

struct Aggregate_stats_in_event
    : public Event
{
    static const std::string static_get_name();

private:
    Aggregate_stats_in_event();
};

struct Desc_stats_in_event
    : public Event
{
    static const std::string static_get_name();

private:
    Desc_stats_in_event();
};
