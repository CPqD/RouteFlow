#ifndef OPENFLOW_DEFAULT
#define OPENFLOW_DEFAULT
/* Copyright 2009 (C) Stanford University
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
#include "openflow/openflow.h"

/** Send flow removed messages
 */
#define SEND_FLOW_REMOVED true

/** Default idle timeout for flows
 */
#define DEFAULT_FLOW_TIMEOUT 5

/** Provide default flags for flow_mod
 */
uint16_t ofd_flow_mod_flags()
{
  uint16_t flag = 0;
  if (SEND_FLOW_REMOVED)
    flag += OFPFF_SEND_FLOW_REM;
  
  return flag;
}

#endif
