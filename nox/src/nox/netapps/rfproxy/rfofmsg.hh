#ifndef __RFOFMSG_H__
#define __RFOFMSG_H__

#include <stdio.h>
#include <arpa/inet.h>
#include <boost/shared_array.hpp>
#include <cstring>
#include <linux/if_ether.h>

#include "openflow/openflow.h"
#include "defs.h"

#define OFP_BUFFER_NONE (0xffffffff)

#define IPPROTO_OSPF    0x59
#define IPADDR_RIPv2    (inet_addr("224.0.0.9"))
#define TPORT_BGP       0x00B3
#define TPORT_LDP       0x286

typedef uint8_t* MSG;
MSG msg_new(uint8_t* src, size_t size);
size_t msg_size(MSG msg);
void msg_save(MSG msg, const char* fname);
void msg_delete(MSG msg);

uint32_t ofp_get_mask(struct in_addr, int shift);
uint32_t ofp_get_mask(uint8_t, int shift);

void ofm_init(ofp_flow_mod* ofm, size_t size);
void ofm_match_in(ofp_flow_mod* ofm, uint16_t in);
void ofm_match_dl(ofp_flow_mod* ofm, uint32_t match, uint16_t type, const uint8_t src[], const uint8_t dst[]);
void ofm_match_vlan(ofp_flow_mod* ofm, uint32_t match, uint16_t id, uint8_t priority);
void ofm_match_nw(ofp_flow_mod* ofm, uint32_t match, uint8_t proto, uint8_t tos, uint32_t src, uint32_t dst);
void ofm_match_tp(ofp_flow_mod* ofm, uint32_t match, uint16_t src, uint16_t dst);
void ofm_set_action(ofp_action_header* hdr, uint16_t type, uint16_t port, const uint8_t addr[]);

void ofm_set_command(ofp_flow_mod* ofm, enum ofp_flow_mod_command cmd);
void ofm_set_command(ofp_flow_mod* ofm, uint16_t cmd, uint32_t id, uint16_t idle_to, uint16_t hard_to, uint16_t port);

MSG create_config_msg(DATAPATH_CONFIG_OPERATION operation);
// TODO: mask doesn't need to be an uint32_t in these three functions
MSG create_flow_install_msg(uint32_t ip, uint32_t mask, uint8_t srcMac[], uint8_t dstMac[], uint32_t dstPort);
MSG create_flow_remove_msg(uint32_t ip, uint32_t mask, uint8_t srcMac[]);
MSG create_temporary_flow_msg(uint32_t ip, uint32_t mask, uint8_t srcMac[]);

#endif /* __RFOFMSG_H__ */
