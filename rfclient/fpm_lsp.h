#ifndef FPM_LSP_H_
#define FPM_LSP_H_

#include <netinet/in.h>

typedef enum nhlfe_operation_enum {
  PUSH = 0,
  POP = 1,
  SWAP = 2
} nhlfe_operation_enum_t;

typedef enum table_operation_enum {
  ADD_LSP = 0,
  REMOVE_LSP = 1,
} table_operation_enum_t;

typedef enum ip_version_enum {
  IPv4 = 4,
  IPv6 = 6,
} ip_version_enum_t;

typedef union ip4_or_6_address {
    in_addr_t ipv4;
    struct in6_addr ipv6;
} ip4_or_6_address_t;

/*
 * Next Hop Label Forwarding Entry (NHLFE)
 * Used from FPM protocol version 2
 */
typedef struct nhlfe_msg_t_
{
  /*
   * The IP version (4 or 6) that the next hop field contains.
   */
  uint8_t ip_version;

  /*
   * The next hop IP address. Used to look up the interface for packet output.
   */
  ip4_or_6_address_t next_hop_ip;

  /*
   * Flow table operation, eg. add, delete, modify.
   */
  uint8_t table_operation;

  /*
   * Label operation, eg. push, pop, swap.
   */
  uint8_t nhlfe_operation;

  /*
   * Incoming label to match on. Network byte-order.
   */
  uint32_t in_label;

  /*
   * Outgoing label to write. Network byte-order.
   */
  uint32_t out_label;
} __attribute__((packed)) nhlfe_msg_t;

typedef struct ftn_msg_t_
{
  /*
   * The IP version (4 or 6) that the next hop field contains.
   */
  uint8_t ip_version;

  /*
   * Prefix length for matching
   */
  uint8_t mask;

  /*
   * Flow table operation, eg. add, delete, modify.
   */
  uint8_t table_operation;

  /*
   * Incoming IP to match on.
   */
  ip4_or_6_address_t match_network;

  /*
   * Outgoing label to write. Network byte-order.
   */
  uint32_t out_label;

  /*
   * The next hop IP address. Used to look up the interface for packet output.
   */
  ip4_or_6_address_t next_hop_ip;
} __attribute__((packed)) ftn_msg_t;
#endif  /* FPM_LSP_H_ */
