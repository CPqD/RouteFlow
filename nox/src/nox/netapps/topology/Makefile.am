include ../../../Make.vars 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

EXTRA_DIST =\
	__init__.py \
	pytopology_test.py \
	pytopology.i \
	meta.json

pkglib_LTLIBRARIES =		\
	topology.la	

topology_la_CPPFLAGS =								\
	$(AM_CPPFLAGS)								\
	-I$(top_srcdir)/src/nox							\
	-I$(top_srcdir)/src/nox/netapps \
	-I$(top_srcdir)/src/nox/coreapps \
	-D__COMPONENT_FACTORY_FUNCTION__=topology_get_factory

topology_la_SOURCES = topology.cc topology.hh
topology_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

if PY_ENABLED
NOX_RUNTIMEFILES += __init__.py\
	pytopology_test.py

pytopology_wrap.cc pytopology.py: pytopology.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps/ -outdir $(builddir)/. -o pytopology_wrap.cc  -module pytopology $(srcdir)/pytopology.i

pkglib_LTLIBRARIES += _pytopology.la
_pytopology_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pytopology_la_SOURCES = pytopology.cc pytopology.hh pytopology.i
nodist__pytopology_la_SOURCES = pytopology_wrap.cc
_pytopology_la_LDFLAGS = -module

NOX_PYLIBFILES = _pytopology.so
NOX_PYBUILDFILES = pytopology_wrap.cc pytopology.py
endif # PY_ENABLED

all-local: nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
