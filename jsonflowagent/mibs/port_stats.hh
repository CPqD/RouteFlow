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
#ifndef JFA_PORT_STATS_HH
#define JFA_PORT_STATS_HH

#include "mib.hh"

enum PS_COLUMN {
  PS_RXPACKETS = 1,
  PS_TXPACKETS = 2,
  PS_RXBYTES = 3,
  PS_TXBYTES = 4,
  PS_RXDROPPED = 5,
  PS_TXDROPPED = 6,
  PS_RXERRORS = 7,
  PS_TXERRORS = 8,
  PS_RXFRAMEERR = 9,
  PS_RXOVERERR = 10,
  PS_RXCRCERR = 11,
  PS_COLLISIONS = 12,
  PS_COLUMN_MAX = 13,
};

/**
 * Given that we are representing our OID exactly the same as our JSON
 * messages, we don't need this structure; an array of counter64 will do.
 */
/*typedef struct port_stats {
  struct counter64 rx_packets;
  struct counter64 tx_packets;
  struct counter64 rx_bytes;
  struct counter64 tx_bytes;
  struct counter64 rx_dropped;
  struct counter64 tx_dropped;
  struct counter64 rx_errors;
  struct counter64 tx_errors;
  struct counter64 rx_frame_err;
  struct counter64 rx_over_err;
  struct counter64 rx_crc_err;
  struct counter64 collisions;
} port_stats_t;*/

void init_port_stats(void);
void ps_set(uint64_t dpid, uint16_t port, struct counter64 phy[]);

#endif /* JFA_PORT_STATS_HH */
