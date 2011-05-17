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
%{
#include "bindings_storage/bindings_storage.hh"

using namespace vigil;
using namespace vigil::applications;
%}

struct Name {
  enum Type { NONE = 0, LOCATION, HOST, USER, SWITCH, PORT, LOC_TUPLE,
          LOCATION_GROUP, HOST_GROUP, USER_GROUP, SWITCH_GROUP}; 
  Name(const string &n, Type t, int64_t i=-1);  
  string name;
  Type name_type; 
  int64_t id;
  bool operator==(const Name &o) const;
  bool operator<(const Name &o) const;
}; 

struct Loc {
  enum PortType { NO_PORT = -1 }; 
  Loc(const datapathid &d, uint16_t p);
  datapathid dpid;
  uint16_t port; 
  bool operator==(const Loc &o) const;
  bool operator<(const Loc &o) const;
}; 


