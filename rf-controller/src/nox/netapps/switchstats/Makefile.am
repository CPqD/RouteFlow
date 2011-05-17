include ../../../Make.vars 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

EXTRA_DIST =\
	meta.json\
	__init__.py\
	switchstats.py

NOX_RUNTIMEFILES = meta.json	\
	__init__.py		\
	switchstats.py

pkglib_LTLIBRARIES = \
	cswitchstats.la		

cswitchstats_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../ \
	-I$(top_srcdir)/src/nox \
	-I$(top_srcdir)/src/nox/netapps \
	-I$(top_srcdir)/src/nox/coreapps  

cswitchstats_la_SOURCES = cswitchstats.cc cswitchstats.hh event-tracker.hh
cswitchstats_la_LDFLAGS = -module -export-dynamic

if PY_ENABLED
pycswitchstats_wrap.cc pycswitchstats.py: pycswitchstats.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/coreapps/ -I$(top_srcdir)/src/nox/lib/ -outdir $(builddir)/. -o pycswitchstats_wrap.cc  -module pycswitchstats $(srcdir)/pycswitchstats.i

pkglib_LTLIBRARIES += _pycswitchstats.la
_pycswitchstats_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pycswitchstats_la_SOURCES = cswitchstats_proxy.cc cswitchstats_proxy.hh
nodist__pycswitchstats_la_SOURCES = pycswitchstats_wrap.cc
_pycswitchstats_la_LDFLAGS = -module

NOX_PYLIBFILES = _pycswitchstats.so

EXTRA_DIST +=\
	pycswitchstats.i

NOX_PYBUILDFILES =  \
	pycswitchstats.py

endif # PY_ENABLED


all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
