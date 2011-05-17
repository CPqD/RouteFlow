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

#include "flow_in.hh"

#include "vlog.hh"

namespace vigil {

static Vlog_module lg("flow_in");

Flow_in_event::Flow_in_event(const timeval& received_,
                             const Packet_in_event& pi,
                             const Flow& flow_)
    : Event(static_get_name()), active(true), fn_applied(false),
      received(received_), datapath_id(pi.datapath_id), flow(flow_),
      buf(pi.buf), total_len(pi.total_len), buffer_id(pi.buffer_id),
      reason(pi.reason), dst_authed(false), routed_to(NOT_ROUTED)
{ }

Flow_in_event::Flow_in_event()
    : Event(static_get_name()), active(true), fn_applied(false),
      total_len(0), dst_authed(false), routed_to(NOT_ROUTED)
{ }

#ifdef TWISTED_ENABLED

template <>
PyObject*
to_python(const Flow_in_event::DestinationInfo& dinfo)
{
    PyObject *pydinfo = PyDict_New();
    if (pydinfo == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for destination info.");
        Py_RETURN_NONE;
    }
    pyglue_setdict_string(pydinfo, "authed_location", to_python(dinfo.authed_location));
    pyglue_setdict_string(pydinfo, "allowed", to_python(dinfo.allowed));
    pyglue_setdict_string(pydinfo, "waypoints", to_python_list(dinfo.waypoints));
    pyglue_setdict_string(pydinfo, "rules", to_python_list(dinfo.rules));
    return pydinfo;
}

template <>
PyObject*
to_python(const Flow_in_event::DestinationList& dlist)
{
    PyObject *pydinfos = PyList_New(dlist.size());
    if (pydinfos == NULL) {
        VLOG_ERR(lg, "Could not create Python list for destinations.");
        Py_RETURN_NONE;
    }

    Flow_in_event::DestinationList::const_iterator dinfo = dlist.begin();
    for (uint32_t i = 0; dinfo != dlist.end(); ++i, ++dinfo) {
        if (PyList_SetItem(pydinfos, i, to_python(*dinfo)) != 0) {
            VLOG_ERR(lg, "Could not set destination info list.");
        }
    }
    return pydinfos;
}

PyObject*
route_source_to_python(const boost::shared_ptr<Location>& location)
{
    if (location == NULL) {
        Py_RETURN_NONE;
    }
    return to_python(location->sw->dp.as_host()
                     | (((uint64_t)location->port) << 48));
}

PyObject*
route_destinations_to_python(const std::vector<boost::shared_ptr<Location> >& dlist)
{
    PyObject *pydlist = PyList_New(dlist.size());
    if (pydlist == NULL) {
        VLOG_ERR(lg, "Could not create list for route destinations.");
        Py_RETURN_NONE;
    }

    std::vector<boost::shared_ptr<Location> >::const_iterator loc = dlist.begin();
    for (uint32_t i = 0; loc != dlist.end(); ++i, ++loc) {
        uint64_t dpport = (*loc)->sw->dp.as_host()
            | (((uint64_t)(*loc)->port) << 48);
        if (PyList_SetItem(pydlist, i, to_python(dpport)) != 0) {
            VLOG_ERR(lg, "Could not set route destination list item.");
        }
    }
    return pydlist;
}

#endif

}
