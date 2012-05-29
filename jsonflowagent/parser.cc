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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <sys/types.h>
#include <iostream>
#include "cJSON.h"
#include "parser.hh"
#include "mibs/phy_port.hh"
#include "mibs/port_stats.hh"

#define STATISTIC_UNSUPPORTED 0xffffffffffffffffL

void parse_features_reply(cJSON *datapaths);
void parse_port_stats(cJSON *datapaths);

/**
 *  convert_to_counter64():
 *
 * Convertor for Net-SNMP counter64 structures
 * Works around the lack of 64-bit support in Net-SNMP
 */
void convert_to_counter64(struct counter64 *dst, uint64_t src) {
  dst->high = src >> 32;
  dst->low = src;
}

/**
 * Parse the JSON in the given buf.
 *
 * Note: Here we use cJSON library, which is known to segfault on invalid
 * input. In the future we may want to consider replacing cJSON with something
 * which is more stable.
 */
void parse_json(char *buf, size_t len) {
  cJSON *root = cJSON_Parse(buf);
  cJSON *json_type = cJSON_GetObjectItem(root, "type");

  if (json_type == NULL) {
    fprintf(stderr, "Failed to get \"type\" from msg");
    return;
  }

  char *type = json_type->valuestring;
  if (type == NULL) {
    fprintf(stderr, "json msg type: NULL\n");
    return;
  }

  if (strncmp(type, "features_reply", strlen("features_reply")) == 0) {
    parse_features_reply(cJSON_GetObjectItem(root, "datapaths"));
  } else if (strncmp(type, "port_stats", strlen("port_stats")) == 0) {
    parse_port_stats(cJSON_GetObjectItem(root, "datapaths"));
  } else {
    fprintf(stderr, "Unrecognised jsonstats msg type: %s", type);
  }
}

void parse_features_reply(cJSON *datapaths) {
  int dp_iter, port_iter;
  struct phy_port phy;

  /* For each datapath in our msg, we want to parse each set of port stats */
  for (dp_iter = 0; dp_iter < cJSON_GetArraySize(datapaths); ++dp_iter) {
    cJSON *dp = cJSON_GetArrayItem(datapaths, dp_iter);
    cJSON *ports = cJSON_GetObjectItem(dp, "ports");
    uint64_t dpid =
      strtoul(cJSON_GetObjectItem(dp, "datapath_id")->valuestring, NULL, 0);

    for (port_iter = 0; port_iter < cJSON_GetArraySize(ports); ++port_iter) {
      cJSON *port_info = cJSON_GetArrayItem(ports, port_iter);
      uint16_t port_no = 0;

      memset(&phy, 0, sizeof(struct phy_port));

      /* Parse the features_reply for this port */
      port_no = cJSON_GetObjectItem(port_info, "port_no")->valueint;

      phy.hw_addr = cJSON_GetObjectItem(port_info, "hw_addr")->valuestring;
      phy.hw_addr_len = strlen(phy.hw_addr);
      phy.name = cJSON_GetObjectItem(port_info, "name")->valuestring;
      phy.name_len = strlen(phy.name);

      phy.config = cJSON_GetObjectItem(port_info, "config")->valueint;
      phy.state = cJSON_GetObjectItem(port_info, "state")->valueint;
      phy.curr = cJSON_GetObjectItem(port_info, "curr")->valueint;
      phy.advertised = cJSON_GetObjectItem(port_info, "advertised")->valueint;
      phy.supported = cJSON_GetObjectItem(port_info, "supported")->valueint;
      phy.peer = cJSON_GetObjectItem(port_info, "peer")->valueint;

      phy_set(dpid, port_no, &phy);
    }
  }
}

void parse_port_stats(cJSON *datapaths) {
  int dp_iter, port_iter;

  // ofp_port_stats is essentially an array of 12 64-bit values
  struct counter64 ps[12];

  /* For each datapath in our msg, we want to parse each set of port stats */
  for (dp_iter = 0; dp_iter < cJSON_GetArraySize(datapaths); ++dp_iter) {
    cJSON *dp = cJSON_GetArrayItem(datapaths, dp_iter);
    cJSON *ports = cJSON_GetObjectItem(dp, "ports");
    uint64_t dpid =
      strtoul(cJSON_GetObjectItem(dp, "datapath_id")->valuestring, NULL, 0);

    for (port_iter = 0; port_iter < cJSON_GetArraySize(ports); ++port_iter) {
      cJSON *port_info = cJSON_GetArrayItem(ports, port_iter);
      uint16_t port_no = 0;

      memset(&ps, 0, sizeof(struct counter64) * 12);

      port_no = cJSON_GetObjectItem(port_info, "port_no")->valueint;
      port_info = cJSON_GetObjectItem(port_info, "port_no");

      /* Assume that our incoming JSON is a series of 64bit values */
      for (int i = 0; i < PS_COLUMN_MAX - 1; ++i) {
        port_info = port_info->next;
        convert_to_counter64(&ps[i], strtoul(port_info->valuestring, NULL, 0));
      }

      ps_set(dpid, port_no, ps);
    }
  }
}
