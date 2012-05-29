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
#include <vector>
#include <map>
#include "port_stats.hh"

netsnmp_table_data_set *ps_tset_;

/**
 *  ps_set():
 *
 * Set statistics for the row corresponding to the given datapath,port.
 */
void ps_set(uint64_t datapath, uint16_t port, struct counter64 ps[]) {
  struct snmp_table *table = jfa_get_snmp_table(datapath);

  if (table->ps.size() <= port) {
    table->ps.resize(port + 1);
  }

  /* If row "oid.datapath.port" hasn't been created yet, create it */
  if (table->ps[port] == NULL) {
    table->ps[port] = netsnmp_create_table_data_row();
    netsnmp_table_row_add_index(table->ps[port], ASN_INTEGER,
                                &table->index, sizeof(&table->index));
    netsnmp_table_row_add_index(table->ps[port], ASN_INTEGER,
                                &port, sizeof(port));
    netsnmp_table_dataset_add_row(ps_tset_, table->ps[port]);
  }

  /* Because our SNMP OID matches the JSON message structure, we can just
   * iterate across our array instead of dealing with individual values. */
  for (int i = 1; i < PS_COLUMN_MAX; ++i) {
    netsnmp_set_row_column(table->ps[port], i, ASN_COUNTER64,
                           (const char *)&ps[i-1], sizeof(struct counter64));
  }
}

void init_port_stats(void)
{
  static oid port_stats_oid[] = { 1, 3, 6, 1, 4, 1, 13727, 2380, 2 };

  ps_tset_ = netsnmp_create_table_data_set("port_stats");
  netsnmp_table_dataset_add_index(ps_tset_, ASN_INTEGER);

  for (int i = 1; i < PS_COLUMN_MAX; ++i) {
    netsnmp_table_set_add_default_row(ps_tset_, i, ASN_COUNTER64, 1, NULL, 0);
  }

  netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                      ("port_stats", NULL, port_stats_oid,
                                       OID_LENGTH(port_stats_oid),
                                       HANDLER_CAN_RWRITE), ps_tset_, NULL);
}
