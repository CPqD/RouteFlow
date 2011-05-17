include ../../../Make.vars 

CONFIGURE_DEPENCIES = $(srcdir)/Makefile.am

EXTRA_DIST =\
	meta.json

pkglib_LTLIBRARIES =		\
	routeflowc.la

routeflowc_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox \
                                    -I$(top_srcdir)/src/nox/coreapps/routeflowc \
                                    -I$(top_srcdir)/src/nox/netapps \
				    -I$(top_srcdir)/../common/rf-protocol \
				    -I$(top_srcdir)/../include \
				    -I$(top_srcdir)/../common/ipc

routeflowc_la_SOURCES = routeflowc.cc
routeflowc_la_LIBADD = $(top_srcdir)/../build/lib/librf-protocol.a
routeflowc_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local

