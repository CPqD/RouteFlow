/*
 *  Copyright 2012 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#ifndef JFA_PHY_PORT_HH
#define JFA_PHY_PORT_HH

#include "mib.hh"

enum PHY_COLUMN {
  PHY_HWADDR = 1,
  PHY_NAME   = 2,
  PHY_CONFIG = 3,
  PHY_STATE  = 4,
  PHY_CURR   = 5,
  PHY_ADVERTISED = 6,
  PHY_SUPPORTED  = 7,
  PHY_PEER       = 8,
};

typedef struct phy_port {
  char    *hw_addr;
  size_t   hw_addr_len;
  char    *name;
  size_t   name_len;

  uint32_t config;
  uint32_t state;
  uint32_t curr;
  uint32_t advertised;
  uint32_t supported;
  uint32_t peer;
} phy_port_t;

void init_phy_port(void);
void phy_set(uint64_t dpid, uint16_t port, struct phy_port *phy);

#endif /* JFA_PHY_PORT_HH */
