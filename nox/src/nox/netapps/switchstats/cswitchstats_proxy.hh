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
#ifndef CSWITCHSTATS_PROXY_HH__
#define CSWITCHSTATS_PROXY_HH__

#include <Python.h>

#include "cswitchstats.hh"
#include "netinet++/datapathid.hh"

namespace vigil {
namespace applications {

class cswitchstats_proxy{

public: 
    cswitchstats_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    float get_global_conn_p_s(void);
    float get_switch_conn_p_s(datapathid);
    float get_loc_conn_p_s   (datapathid, uint16_t);


protected:   

    CSwitchStats* csstats;
    container::Component* c;

} ; // class cswitchstats_proxy

} // namespace applications
} // namespace vigil

#endif  //  CSWITCHSTATS_PROXY_HH__
