include ../../../Make.vars 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

EXTRA_DIST =\
	meta.json\
	__init__.py

NOX_RUNTIMEFILES = meta.json

if PY_ENABLED
NOX_RUNTIMEFILES += __init__.py	
endif # PY_ENABLED

pkglib_LTLIBRARIES = \
	switch_management.la		

switch_management_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../ \
	-I$(top_srcdir)/src/nox \
	-I$(top_srcdir)/src/nox/netapps \
	-I$(top_srcdir)/src/nox/coreapps  

switch_management_la_SOURCES = switch_management.cc switch_management.hh
switch_management_la_LDFLAGS = -module -export-dynamic

if PY_ENABLED
pyswitch_management_wrap.cc pyswitch_management.py: pyswitch_management.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/coreapps/ -I$(top_srcdir)/src/nox/lib/ -outdir $(builddir)/. -o pyswitch_management_wrap.cc  -module pyswitch_management $(srcdir)/pyswitch_management.i

pkglib_LTLIBRARIES += _pyswitch_management.la
_pyswitch_management_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pyswitch_management_la_SOURCES = switch_management_proxy.cc switch_management_proxy.hh
nodist__pyswitch_management_la_SOURCES = pyswitch_management_wrap.cc
_pyswitch_management_la_LDFLAGS = -module

NOX_PYLIBFILES = _pyswitch_management.so

EXTRA_DIST +=\
	pyswitch_management.i

NOX_PYBUILDFILES =  \
	pyswitch_management.py

endif # PY_ENABLED


all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
