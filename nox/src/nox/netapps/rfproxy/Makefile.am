include ../../../Make.vars 

EXTRA_DIST =\
	meta.json

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	rfproxy.la

rfproxy_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox \
                        -I$(top_srcdir)/src/nox/netapps/ \
                        -I$(top_srcdir)/../rflib \
                        -I$(top_srcdir)/../rflib/ipc \
                        -I$(top_srcdir)/../rflib/rftable \
                        -I$(top_srcdir)/../rflib/types

rfproxy_la_SOURCES = rfproxy.hh rfproxy.cc OFInterface.hh OFInterface.cc \
                     rfofmsg.hh rfofmsg.cc
rfproxy_la_LIBADD = $(top_srcdir)/../build/lib/rflib.a
rfproxy_la_LDFLAGS = -module -export-dynamic -lmongoclient

NOX_RUNTIMEFILES = meta.json	

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
