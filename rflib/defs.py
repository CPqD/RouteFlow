MONGO_ADDRESS = "localhost:27017"
MONGO_DB_NAME = "db"

RFCLIENT_RFSERVER_CHANNEL = "rfclient<->rfserver"
RFSERVER_RFPROXY_CHANNEL = "rfserver<->rfproxy"

RF_TABLE_NAME = "rftable"

RFSERVER_ID = "rfserver"
RFPROXY_ID = "rfproxy"

DEFAULT_RFCLIENT_INTERFACE = "eth0"

FULL_IPV4_MASK = "255.255.255.255"
EMPTY_MAC_ADDRESS = "00:00:00:00:00:00"

RFVS_DPID = 0x7266767372667673

MATCH_L2 = True

DC_DROP_ALL = 0			# Drop all incoming packets.
DC_CLEAR_FLOW_TABLE = 1 # Clear flow table.
DC_VM_INFO = 2		    # Flow to communicate two linked VMs.
DC_RIPV2 = 3			# RIPv2 protocol.
DC_OSPF = 4 			# OSPF protocol.
DC_BGP = 5			    # BGP protocol.
DC_ARP = 6			    # ARP protocol.
DC_ICMP = 7 			# ICMP protocol.
DC_ALL = 8				# Send all traffic to the controller.

# TODO: move these to their proper locations as developments furthers around POX
# RFTable
ID = "_id"
VM_ID = "vm_id"
VM_PORT = "vm_port"
VS_ID = "vs_id"
VS_PORT = "vs_port"
DP_ID = "dp_id"
DP_PORT = "dp_port"

