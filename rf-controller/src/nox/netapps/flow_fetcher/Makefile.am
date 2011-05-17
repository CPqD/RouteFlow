include ../../../Make.vars 

EXTRA_DIST = 					\
	meta.json				\
	__init__.py				\
	test.py \
	pyflow_fetcher.i

pkglib_LTLIBRARIES = flow_fetcher.la 

flow_fetcher_la_CPPFLAGS =						\
	$(AM_CPPFLAGS)							\
	-I $(top_srcdir)/src/nox					\
	-I $(top_srcdir)/src/include					\
	-I $(top_srcdir)/src/nox/coreapps  \
	-I $(top_srcdir)/src/nox/netapps \
	-D__COMPONENT_FACTORY_FUNCTION__=flow_fetcher_get_factory

flow_fetcher_la_SOURCES = flow_fetcher.cc flow_fetcher.hh 
flow_fetcher_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

if PY_ENABLED

AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
NOX_RUNTIMEFILES += __init__.py test.py
NOX_PYBUILDFILES = pyflow_fetcher.py 

pyflow_fetcher_wrap.cc pyflow_fetcher.py: pyflow_fetcher.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps/ -outdir $(builddir)/. -o pyflow_fetcher_wrap.cc -module pyflow_fetcher $(srcdir)/pyflow_fetcher.i

pkglib_LTLIBRARIES += _pyflow_fetcher.la
_pyflow_fetcher_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps
_pyflow_fetcher_la_SOURCES = flow_fetcher_proxy.cc flow_fetcher_proxy.hh
nodist__pyflow_fetcher_la_SOURCES = pyflow_fetcher_wrap.cc
_pyflow_fetcher_la_LDFLAGS = -module

NOX_PYLIBFILES = _pyflow_fetcher.so

endif # PY_ENABLED

all-local: nox-runtimefiles-build nox-all-local
clean-local: nox-runtimefiles-clean nox-clean-local
install-exec-hook: nox-install-local
