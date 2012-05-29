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

#include "mib.hh"

snmp_table_t *jfa_get_snmp_table(uint64_t datapath) {
  std::map<uint64_t,snmp_table_t>::iterator iter;

  iter = mib_tables.find(datapath);

  if (iter != mib_tables.end()) {
    return &iter->second;
  }

  snmp_table_t table;
  table.index = mib_tables.size() + 1;
  return &mib_tables.insert(std::make_pair(datapath,table)).first->second;
}
