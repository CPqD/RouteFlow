include ../../../Make.vars 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)

pkglib_LTLIBRARIES =		\
	simplerouting.la        \
	routeinstaller.la	\
	flowroute_record.la

routeinstaller_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps -I $(top_srcdir)/src/nox/coreapps
routeinstaller_la_SOURCES = routeinstaller.cc routeinstaller.hh
routeinstaller_la_LDFLAGS = -module -export-dynamic

flowroute_record_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
flowroute_record_la_SOURCES = flowroute_record.hh flowroute_record.cc
flowroute_record_la_LDFLAGS = -module -export-dynamic

simplerouting_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/ -I $(top_srcdir)/src/nox/coreapps
simplerouting_la_SOURCES = simplerouting.hh simplerouting.cc
simplerouting_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json	

endif # PY_ENABLED


EXTRA_DIST = \
	meta.json

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
