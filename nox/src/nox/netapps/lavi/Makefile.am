include ../../../Make.vars 

EXTRA_DIST =\
	meta.json

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	lavi_hostflow.la \
	lavitest_showflow.la \
	lavi_host2sw.la \
	lavi_hosts.la \
	lavi_networkflow.la \
	lavi_flows.la \
	lavi_swlinks.la \
	lavi_links.la \
	lavi_nodes.la \
	lavi_switches.la \
	lavi.la

lavi_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
lavi_la_SOURCES = lavi.hh lavi.cc
lavi_la_LDFLAGS = -module -export-dynamic

lavi_nodes_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_nodes_la_SOURCES = lavi_nodes.hh lavi_nodes.cc
lavi_nodes_la_LDFLAGS = -module -export-dynamic

lavi_switches_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_switches_la_SOURCES = lavi_switches.hh lavi_switches.cc
lavi_switches_la_LDFLAGS = -module -export-dynamic

lavi_links_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_links_la_SOURCES = lavi_links.hh lavi_links.cc
lavi_links_la_LDFLAGS = -module -export-dynamic

lavi_swlinks_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_swlinks_la_SOURCES = lavi_swlinks.hh lavi_swlinks.cc
lavi_swlinks_la_LDFLAGS = -module -export-dynamic

lavi_flows_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_flows_la_SOURCES = lavi_flows.hh lavi_flows.cc
lavi_flows_la_LDFLAGS = -module -export-dynamic

lavi_networkflow_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_networkflow_la_SOURCES = lavi_networkflow.hh lavi_networkflow.cc
lavi_networkflow_la_LDFLAGS = -module -export-dynamic

lavi_hosts_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_hosts_la_SOURCES = lavi_hosts.hh lavi_hosts.cc
lavi_hosts_la_LDFLAGS = -module -export-dynamic

lavi_host2sw_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_host2sw_la_SOURCES = lavi_host2sw.hh lavi_host2sw.cc
lavi_host2sw_la_LDFLAGS = -module -export-dynamic

lavitest_showflow_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavitest_showflow_la_SOURCES = lavitest_showflow.hh lavitest_showflow.cc
lavitest_showflow_la_LDFLAGS = -module -export-dynamic

lavi_hostflow_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
lavi_hostflow_la_SOURCES = lavi_hostflow.hh lavi_hostflow.cc
lavi_hostflow_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json	

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
