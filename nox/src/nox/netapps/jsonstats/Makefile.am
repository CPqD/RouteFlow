include ../../../Make.vars

CONFIGURE_DEPENCIES = $(srcdir)/Makefile.am

EXTRA_DIST =\
	meta.json

pkglib_LTLIBRARIES = jsonstats.la

jsonstats_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox \
                                    -I$(top_srcdir)/src/nox/netapps/jsonstats \
                                    -I$(top_srcdir)/src/nox/coreapps/messenger \
                                    -I$(top_srcdir)/src/nox/netapps \
                                    -I$(top_srcdir)/../include

jsonstats_la_SOURCES = stats_stream.cc jsonstats.cc
jsonstats_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

all-local: nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
