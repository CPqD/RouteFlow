include ../../../Make.vars 

EXTRA_DIST =\
	meta.json

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	linkload.la \
	switchrtt.la \
	datapathmem.la

datapathmem_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
datapathmem_la_SOURCES = datapathmem.hh datapathmem.cc
datapathmem_la_LDFLAGS = -module -export-dynamic

switchrtt_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
switchrtt_la_SOURCES = switchrtt.hh switchrtt.cc
switchrtt_la_LDFLAGS = -module -export-dynamic

linkload_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
linkload_la_SOURCES = linkload.hh linkload.cc
linkload_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json	

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
