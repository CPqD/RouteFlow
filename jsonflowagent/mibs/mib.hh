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
#ifndef JFA_COMMON_HH
#define JFA_COMMON_HH

#include <map>
#include <vector>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/**
 * Structure to bind a datapath index (based on join order) to its data
 */
typedef struct snmp_table {
  uint32_t index;
  std::vector<netsnmp_table_row*> phy; // ofp_phy_port
  std::vector<netsnmp_table_row*> ps;  // ofp_port_stats
} snmp_table_t;

static std::map<uint64_t,snmp_table_t> mib_tables;

/**
 * Get the SNMP table that matches the given datapath.
 * Allocates a new one if it doesn't exist yet.
 */
snmp_table_t *jfa_get_snmp_table(uint64_t datapath);

#endif /*JFA_COMMON_HH*/
