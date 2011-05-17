include ../../../Make.vars 

EXTRA_DIST = \
	__init__.py\
	meta.json\
	discovery.py\
	discoveryws.py\
	link.i


pkglib_LTLIBRARIES =		\
	link_event.la

link_event_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox
link_event_la_SOURCES = link-event.cc
link_event_la_LDFLAGS = -module -export-dynamic

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

noinst_HEADERS =		\
	link-event.hh

NOX_RUNTIMEFILES = meta.json	

if PY_ENABLED
NOX_RUNTIMEFILES += \
	__init__.py		\
	discoveryws.py  \
	discovery.py

NOX_PYBUILDFILES = \
	pylinkevent_wrap.cc \
	pylinkevent.py

link_wrap_includes = \
	link.i

pylinkevent_wrap.cc pylinkevent.py: $(link_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps -outdir $(builddir)/. -o pylinkevent_wrap.cc -module pylinkevent $(srcdir)/link.i
pkglib_LTLIBRARIES += _pylinkevent.la

_pylinkevent_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pylinkevent_la_SOURCES = link-event.hh
nodist__pylinkevent_la_SOURCES = pylinkevent_wrap.cc pylinkevent.py
_pylinkevent_la_LDFLAGS = -module


NOX_PYLIBFILES = _pylinkevent.so
endif # PY_ENABLED


all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
