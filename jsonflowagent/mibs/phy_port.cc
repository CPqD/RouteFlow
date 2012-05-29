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
#include "phy_port.hh"

netsnmp_table_data_set *phy_tset_;

/**
 *  phy_set():
 *
 * Set statistics for the row corresponding to the given datapath,port.
 */
void phy_set(uint64_t datapath, uint16_t port, struct phy_port *phy) {
  struct snmp_table *table = jfa_get_snmp_table(datapath);

  if (table->phy.size() <= port) {
    table->phy.resize(port + 1);
  }

  /* If row "oid.datapath.port" hasn't been created yet, create it */
  if (table->phy[port] == NULL) {
    table->phy[port] = netsnmp_create_table_data_row();
    netsnmp_table_row_add_index(table->phy[port], ASN_INTEGER,
                                &table->index, sizeof(&table->index));
    netsnmp_table_row_add_index(table->phy[port], ASN_INTEGER,
                                &port, sizeof(port));
    netsnmp_table_dataset_add_row(phy_tset_, table->phy[port]);
  }

  netsnmp_set_row_column(table->phy[port], PHY_HWADDR, ASN_OCTET_STR,
                           phy->hw_addr, phy->hw_addr_len);
  netsnmp_set_row_column(table->phy[port], PHY_NAME, ASN_OCTET_STR,
                           phy->name, phy->name_len);
  netsnmp_set_row_column(table->phy[port], PHY_CONFIG,
                           ASN_INTEGER, (const char *)&phy->config,
                           sizeof(int));
  netsnmp_set_row_column(table->phy[port], PHY_STATE,
                           ASN_INTEGER, (const char *)&phy->state,
                           sizeof(int));
  netsnmp_set_row_column(table->phy[port], PHY_CURR,
                           ASN_INTEGER, (const char *)&phy->curr,
                           sizeof(int));
  netsnmp_set_row_column(table->phy[port], PHY_ADVERTISED,
                           ASN_INTEGER, (const char *)&phy->advertised,
                           sizeof(int));
  netsnmp_set_row_column(table->phy[port], PHY_SUPPORTED,
                           ASN_INTEGER, (const char *)&phy->supported,
                           sizeof(int));
  netsnmp_set_row_column(table->phy[port], PHY_PEER,
                           ASN_INTEGER, (const char *)&phy->peer,
                           sizeof(int));
}

void
init_phy_port(void)
{
  static oid phy_port_oid[] = { 1, 3, 6, 1, 4, 1, 13727, 2380, 1 };

  phy_tset_ = netsnmp_create_table_data_set("phy_port");
  netsnmp_table_dataset_add_index(phy_tset_, ASN_INTEGER);

  netsnmp_table_set_multi_add_default_row(phy_tset_,
                  PHY_HWADDR, ASN_OCTET_STR, 1, NULL, 0,
                  PHY_NAME, ASN_OCTET_STR, 1, NULL, 0,
                  PHY_CONFIG, ASN_INTEGER, 1, NULL, 0,
                  PHY_STATE, ASN_INTEGER, 1, NULL, 0,
                  PHY_CURR, ASN_INTEGER, 1, NULL, 0,
                  PHY_ADVERTISED, ASN_INTEGER, 1, NULL, 0,
                  PHY_SUPPORTED, ASN_INTEGER, 1, NULL, 0,
                  PHY_PEER, ASN_INTEGER, 1, NULL, 0,
                  0);

  netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                      ("phy_port", NULL, phy_port_oid,
                                       OID_LENGTH(phy_port_oid),
                                       HANDLER_CAN_RWRITE), phy_tset_, NULL);
}
