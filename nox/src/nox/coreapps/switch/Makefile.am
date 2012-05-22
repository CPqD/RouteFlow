include ../../../Make.vars 

CONFIGURE_DEPENCIES = $(srcdir)/Makefile.am

EXTRA_DIST = \
	meta.json

pkglib_LTLIBRARIES =		\
	switch.la	

switch_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox
switch_la_SOURCES = switch.cc
switch_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
