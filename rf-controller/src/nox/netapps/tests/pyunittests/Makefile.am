include ../../../../Make.vars

if PY_ENABLED

NOX_RUNTIMEFILES =  \
	__init__.py \
	bs_link_test.py\
	controller_test.py\
	componentws_test.py\
	dhcp_parse_test.py\
	dns_parse_test.py\
	eap_parse_test.py\
	ethernet_parse_test.py\
	event_test.py\
	icmp_parse_test.py\
	ipv4_parse_test.py\
	lldp_parse_test.py\
	mod_test.py\
	routing_test.py\
	simple_async_test.py\
	tcp_parse_test.py\
	transactional_storage_test.py\
	udp_parse_test.py\
	util_parse_test.py\
	vlan_parse_test.py\
	webservice_test.py

EXTRA_DIST = $(NOX_RUNTIMEFILES)

endif # PY_ENABLED

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
