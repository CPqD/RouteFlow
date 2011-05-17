include ../../../Make.vars 

EXTRA_DIST = 			\
	__init__.py		\
	meta.json		\
	bootstrap.py    	\
	componentws.py		\
	deferredcallback.i	\
	oxidereactor.cc		\
	pyoxidereactor.py

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)

NOX_RUNTIMEFILES = 		\
	__init__.py		\
	bootstrap.py   		\
	componentws.py		\
	meta.json		\
	pyoxidereactor.py

NOX_PYBUILDFILES = \
	oxidereactor.py\
	deferredcallback.py \
	pycomponent.py

NOX_PYLIBFILES =		\
	pyrt.so \
	_deferredcallback.so	\
	_oxidereactor.so	\
	_pycomponent.so

pkglib_LTLIBRARIES =		\
	pyrt.la			\
	_deferredcallback.la	\
	_oxidereactor.la	\
	_pycomponent.la

pyrt_la_CPPFLAGS =						\
	$(AM_CPPFLAGS) 						\
	-I$(srcdir)/../ 					\
	-I$(top_srcdir)/src/nox					\
	-I$(top_builddir)/src/nox				\
	-D__COMPONENT_FACTORY_FUNCTION__=pyrt_get_factory

pyrt_la_SOURCES =		\
	deferredcallback.cc	\
	deferredcallback.hh	\
	pyrt.cc			\
	pyrt.hh			\
	pycontext.hh		\
	pycomponent.cc		\
	pycomponent.hh		\
	pycontext.cc		\
	pyevent.hh		\
	pyglue.cc		\
	pyglue.hh

nodist_pyrt_la_SOURCES = swigpyrun.h
pyrt_la_LDFLAGS = -module -export-dynamic

component_wrap_includes = 	\
	component.i		\
	context.i 		\
	kernel.i        	\
	event.i		

deferredcallback_wrap.cc deferredcallback.py: deferredcallback.i
	$(SWIG) $(SWIG_PYTHON_OPT) -outdir $(builddir)/. -o deferredcallback_wrap.cc -module deferredcallback $(srcdir)/deferredcallback.i
_deferredcallback_la_SOURCES = deferredcallback.hh
nodist__deferredcallback_la_SOURCES = deferredcallback_wrap.cc
_deferredcallback_la_LDFLAGS = -module
_deferredcallback_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox

pycomponent_wrap.cc pycomponent.py: $(component_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT) $(OPENFLOW_CPPFLAGS) -I$(top_srcdir)/src/nox/lib/ -outdir $(builddir)/. -o pycomponent_wrap.cc -module pycomponent $(srcdir)/component.i
_pycomponent_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/
_pycomponent_la_SOURCES = pycomponent.hh component.i context.i event.i kernel.i
nodist__pycomponent_la_SOURCES = pycomponent_wrap.cc
_pycomponent_la_LDFLAGS = -module

oxidereactor_wrap_includes = 	\
	oxidereactor.i

oxidereactor_wrap.cc oxidereactor.py: $(oxidereactor_wrap_includes) oxidereactor.cc
	$(SWIG) $(SWIG_PYTHON_OPT) $(OPENFLOW_CPPFLAGS) -I$(top_srcdir)/src/nox/lib/ -outdir $(builddir)/. -o oxidereactor_wrap.cc -module oxidereactor $(srcdir)/oxidereactor.i

_oxidereactor_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/../ -I$(top_srcdir)/src/nox
_oxidereactor_la_SOURCES = oxidereactor.hh oxidereactor.i 
nodist__oxidereactor_la_SOURCES = oxidereactor_wrap.cc
_oxidereactor_la_LDFLAGS = -module


endif # PY_ENABLED

all-local: nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
