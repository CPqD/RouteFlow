include ../../../Make.vars 

EXTRA_DIST = meta.json		\
	auth.i			\
	flow_util.i		\
	__init__.py

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

pkglib_LTLIBRARIES =		\
	authenticator.la	\
	flowutil.la

flowutil_la_CPPFLAGS =								\
	$(AM_CPPFLAGS)								\
	-I$(srcdir)/../								\
	-I$(top_srcdir)/src/nox							\
	-I$(top_srcdir)/src/nox/coreapps					\
	-D__COMPONENT_FACTORY_FUNCTION__=flowutil_get_factory

flowutil_la_SOURCES = flow_util.cc flow_util.hh flow_fn_map.hh flow_fn_map.cc   \
	flow_in.hh flow_in.cc

flowutil_la_LDFLAGS = -module -export-dynamic

authenticator_la_CPPFLAGS =							\
	$(AM_CPPFLAGS)								\
	-I$(srcdir)/../								\
	-I$(top_srcdir)/src/nox							\
	-I$(top_srcdir)/src/nox/coreapps					\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src				\
	-I$(top_builddir)/src							\
	-I$(top_builddir)/src/nox/netapps					\
	-D__COMPONENT_FACTORY_FUNCTION__=authenticator_get_factory

authenticator_la_SOURCES = authenticator.hh authenticator_modify.cc		\
	authenticator_util.cc host_event.hh authenticator_event.cc		\
	host_event.cc user_event.hh user_event.cc switch_event.hh		\
	switch_event.cc

authenticator_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

if PY_ENABLED
pyflowutil_wrap_includes = flow_util.i

pyflowutil_wrap.cc pyflowutil.py: $(pyflowutil_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT)  -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps/ -outdir $(builddir)/. -o pyflowutil_wrap.cc -module pyflowutil $(srcdir)/flow_util.i

pkglib_LTLIBRARIES += _pyflowutil.la
_pyflowutil_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox/ -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pyflowutil_la_SOURCES = pyflow_util.cc pyflow_util.hh
nodist__pyflowutil_la_SOURCES = pyflowutil_wrap.cc pyflowutil.py
_pyflowutil_la_LDFLAGS = -module

pyauth_wrap_includes = auth.i

pyauth_wrap.cc pyauth.py: $(pyauth_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT)  -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps/ -outdir $(builddir)/. -o pyauth_wrap.cc -module pyauth $(srcdir)/auth.i

pkglib_LTLIBRARIES += _pyauth.la
_pyauth_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox/ -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pyauth_la_SOURCES = pyauth.cc pyauth.hh
nodist__pyauth_la_SOURCES = pyauth_wrap.cc pyauth.py
_pyauth_la_LDFLAGS = -module

NOX_RUNTIMEFILES += __init__.py
NOX_PYBUILDFILES  = pyflowutil.py pyflowutil_wrap.cc pyauth.py pyauth_wrap.cc

NOX_PYLIBFILES = _pyauth.so _pyflowutil.so

endif # PY_ENABLED

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
