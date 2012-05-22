include ../../../Make.vars 

SUBDIRS = t

EXTRA_DIST =\
	__init__.py\
	oui.txt\
    arp.py\
    bpdu.py\
    dhcp.py\
    dns.py\
    eapol.py\
    eap.py\
    ethernet.py\
    icmp.py\
    __init__.py\
    ipv4.py\
    llc.py\
    lldp.py\
    packet_exceptions.py\
    packet_base.py\
    packet_utils.py\
    tcp.py\
    udp.py\
    vlan.py


if PY_ENABLED
NOX_RUNTIMEFILES = \
    arp.py\
    bpdu.py\
    dhcp.py\
	oui.txt\
    dns.py\
    eapol.py\
    eap.py\
    ethernet.py\
    icmp.py\
    __init__.py\
    ipv4.py\
    llc.py\
    lldp.py\
    packet_exceptions.py\
    packet_base.py\
    packet_utils.py\
    tcp.py\
    udp.py\
    vlan.py

endif # PY_ENABLED

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
