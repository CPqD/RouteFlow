#ifndef __DEFS_H__
#define __DEFS_H__

#define MONGO_ADDRESS "192.169.1.1:27017"
#define MONGO_DB_NAME "db"

#define RFCLIENT_RFSERVER_CHANNEL "rfclient<->rfserver"
#define RFSERVER_RFPROXY_CHANNEL "rfserver<->rfproxy"

#define RFSERVER_ID "rfserver"
#define RFPROXY_ID "rfproxy"

#define DEFAULT_RFCLIENT_INTERFACE "eth0"

#define SYSLOGFACILITY LOG_LOCAL7

#define RFVS_PREFIX 0x72667673
#define IS_RFVS(dp_id) !((dp_id >> 32) ^ RFVS_PREFIX)

#define RF_ETH_PROTO 0x0A0A /* RF ethernet protocol */

#define VLAN_HEADER_LEN 4
#define ETH_HEADER_LEN 14
#define ETH_CRC_LEN 4
#define ETH_PAYLOAD_MAX 1500
#define ETH_TOTAL_MAX (ETH_HEADER_LEN + ETH_PAYLOAD_MAX + ETH_CRC_LEN)
#define RF_MAX_PACKET_SIZE (VLAN_HEADER_LEN + ETH_TOTAL_MAX)

// We must match_l2 in order for packets to go up
#define MATCH_L2 true

typedef enum dp_config {
    DC_DROP_ALL,            /* Drop all incoming packets */
    DC_CLEAR_FLOW_TABLE,    /* Clear flow table */
    DC_VM_INFO,             /* Flow to communicate two linked VM's */
    DC_RIPV2,               /* RIPv2 protocol */
    DC_OSPF,                /* OSPF protocol */
    DC_BGP_PASSIVE,         /* BGP protocol */
    DC_BGP_ACTIVE,          /* BGP protocol */
    DC_ARP,                 /* ARP protocol */
    DC_ICMP,                /* ICMP protocol */
    DC_LDP_PASSIVE,         /* LDP protocol */
    DC_LDP_ACTIVE,          /* LDP protocol */
    DC_ALL = 255            /* Send all traffic to the controller */

} DATAPATH_CONFIG_OPERATION;

typedef enum route_mod_type {
	RMT_ADD,			/* Add flow to datapath */
	RMT_DELETE			/* Remove flow from datapath */
	/* Future implementation */
	//RMT_MODIFY		/* Modify existing flow (Unimplemented) */
} RouteModType;

#define PC_MAP 0
#define PC_RESET 1

#endif /* __DEFS_H__ */
