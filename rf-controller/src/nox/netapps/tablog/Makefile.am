include ../../../Make.vars 

EXTRA_DIST =\
	meta.json

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	flowlog.la \
	rttlog.la \
	tablog.la

tablog_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps/
tablog_la_SOURCES = tablog.hh tablog.cc
tablog_la_LDFLAGS = -module -export-dynamic

rttlog_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
rttlog_la_SOURCES = rttlog.hh rttlog.cc
rttlog_la_LDFLAGS = -module -export-dynamic

flowlog_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
flowlog_la_SOURCES = flowlog.hh flowlog.cc
flowlog_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json	

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
