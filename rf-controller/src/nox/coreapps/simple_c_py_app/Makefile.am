include ../../../Make.vars 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)

pkglib_LTLIBRARIES =		\
	simple_cc_py_app.la

simple_cc_py_app_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox
simple_cc_py_app_la_SOURCES = simple_cc_py_app.cc simple_cc_py_app.hh
simple_cc_py_app_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json	

NOX_RUNTIMEFILES += __init__.py

pysimple_cc_py_app_wrap.cc pysimple_cc_py_app.py: pysimple_cc_py_app.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/lib/ -outdir $(builddir)/. -o pysimple_cc_py_app_wrap.cc  -module pysimple_cc_py_app $(srcdir)/pysimple_cc_py_app.i

pkglib_LTLIBRARIES += _pysimple_cc_py_app.la
_pysimple_cc_py_app_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/
_pysimple_cc_py_app_la_SOURCES = simple_cc_py_proxy.cc simple_cc_py_proxy.hh
nodist__pysimple_cc_py_app_la_SOURCES = pysimple_cc_py_app_wrap.cc
_pysimple_cc_py_app_la_LDFLAGS = -module

NOX_PYLIBFILES = _pysimple_cc_py_app.so
endif # PY_ENABLED


EXTRA_DIST = \
	__init__.py \
	pysimple_cc_py_app.i \
	meta.json

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
