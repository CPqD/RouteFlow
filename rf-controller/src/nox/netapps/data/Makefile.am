include ../../../Make.vars 

EXTRA_DIST = \
	__init__.py \
	meta.json \
	datatypes_impl.py \
	datacache_impl.py \
	datatypes.i \
	datacache.i

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

pkglib_LTLIBRARIES = \
	datatypes.la \
	datacache.la								

datatypes_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../ \
	-I$(top_srcdir)/src/nox \
	-I$(top_srcdir)/src/nox/coreapps \
	-D__COMPONENT_FACTORY_FUNCTION__=datatypes_get_factory

datatypes_la_SOURCES = datatypes.hh datatypes.cc

datatypes_la_LDFLAGS = -module -export-dynamic

datacache_la_CPPFLAGS =								\
	$(AM_CPPFLAGS)								\
	-I$(srcdir)/../								\
	-I$(top_srcdir)/src/nox							\
	-I$(top_srcdir)/src/nox/coreapps					\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src				\
	-I$(top_builddir)/src							\
	-I$(top_builddir)/src/nox/netapps					\
	-D__COMPONENT_FACTORY_FUNCTION__=datacache_get_factory

datacache_la_SOURCES =  datacache.hh datacache.cc principal_event.hh		\
	principal_event.cc

datacache_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES =								\
	meta.json

if PY_ENABLED

SWIG_INCLUDES = 								\
	-I$(top_srcdir)/src/nox/lib/ 						\
	-I$(top_srcdir)/src/nox/coreapps/

pydatatypes_wrap_includes = 							\
	datatypes.i

pydatatypes_wrap.cc pydatatypes.py: $(pydatatypes_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT)  $(SWIG_INCLUDES) $(OPENFLOW_CPPFLAGS)	\
	-outdir $(builddir)/. -o pydatatypes_wrap.cc -module pydatatypes	\
	$(srcdir)/datatypes.i

pkglib_LTLIBRARIES += _pydatatypes.la
_pydatatypes_la_CPPFLAGS = 							\
	$(AM_CPPFLAGS) 								\
	-I$(top_srcdir)/src/nox/ 						\
	-I$(srcdir)/../								\
	-I$(top_builddir)/src/nox/ 						\
	-I$(top_srcdir)/src/nox/coreapps/ 					\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src

_pydatatypes_la_SOURCES = pydatatypes.hh pydatatypes.cc

nodist__pydatatypes_la_SOURCES = pydatatypes_wrap.cc
_pydatatypes_la_LDFLAGS = -module

pydatacache_wrap_includes = 							\
	datacache.i

pydatacache_wrap.cc pydatacache.py: $(pydatacache_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT)  $(SWIG_INCLUDES) $(OPENFLOW_CPPFLAGS)	\
	-outdir $(builddir)/. -o pydatacache_wrap.cc -module pydatacache	\
	$(srcdir)/datacache.i

pkglib_LTLIBRARIES += _pydatacache.la
_pydatacache_la_CPPFLAGS = 							\
	$(AM_CPPFLAGS) 								\
	-I$(top_srcdir)/src/nox/ 						\
	-I$(srcdir)/../								\
	-I$(top_builddir)/src/nox/ 						\
	-I$(top_srcdir)/src/nox/coreapps/ 					\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src

_pydatacache_la_SOURCES = pydatacache.hh pydatacache.cc 

nodist__pydatacache_la_SOURCES = pydatacache_wrap.cc
_pydatacache_la_LDFLAGS = -module

NOX_RUNTIMEFILES += \
	__init__.py \
	datatypes_impl.py \
	datacache_impl.py 					

NOX_PYBUILDFILES =							\
	pydatacache.py							\
	pydatatypes.py

NOX_PYLIBFILES = 							\
	_pydatatypes.so							\
	_pydatacache.so

endif # PY_ENABLED

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
