include ../../../Make.vars 

SUBDIRS = t

EXTRA_DIST = \
	meta.json \
	__init__.py	\
	pystorage.cc \
	pystorage.hh \
	storage.i \
	util.py	\
	storage.py \
	pystorage-common.hh 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

NOX_RUNTIMEFILES = meta.json

pkglib_LTLIBRARIES =							\
	storage-common.la						\
	storage-backend.la						\
	storage-memleak-test.la

storage_common_la_CPPFLAGS = 						\
	$(AM_CPPFLAGS) 							\
	-I$(srcdir)/../ 						\
	-I$(top_srcdir)/src/nox/					\
	-I$(top_srcdir)/src/nox/coreapps/				\
	-D__COMPONENT_FACTORY_FUNCTION__=storage_common_get_factory
storage_common_la_SOURCES = 						\
	storage.cc 							\
	storage.hh							\
	storage-blocking.cc						\
	storage-blocking.hh						\
	storage-hash.hh							\
	storage_util.hh			

storage_common_la_LDFLAGS = -module -export-dynamic

storage_backend_la_CPPFLAGS =						\
	$(AM_CPPFLAGS)							\
	-I$(srcdir)/../							\
	-I$(top_srcdir)/src/nox						\
	-I$(top_srcdir)/src/nox/coreapps				\
	-I$(top_builddir)/src						\
	-I$(top_builddir)/src/nox/netapps				\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src			\
	-D__COMPONENT_FACTORY_FUNCTION__=storage_backend_get_factory
storage_backend_la_SOURCES =						\
	dht-storage.cc							\
	dht-storage.hh							\
	dht-impl.cc							\
	dht-impl.hh
storage_backend_la_LDFLAGS = -module -export-dynamic


storage_memleak_test_la_CPPFLAGS =					\
	$(AM_CPPFLAGS)							\
	-I$(srcdir)/../							\
	-I$(top_srcdir)/src/nox						\
	-I$(top_srcdir)/src/nox/coreapps/				\
	-D__COMPONENT_FACTORY_FUNCTION__=storage_memleak_test_get_factory
storage_memleak_test_la_SOURCES =					\
        storage-memleak-test.cc
storage_memleak_test_la_LDFLAGS = -module -export-dynamic

if PY_ENABLED
pystorage_wrap_includes = storage.i

pystorage_wrap.cc pystorage.py:	storage.i
	$(SWIG) $(SWIG_PYTHON_OPT)					\
	-I$(srcdir)/../							\
	-outdir $(builddir)/.						\
	-o pystorage_wrap.cc						\
	-module pystorage $(srcdir)/storage.i
pkglib_LTLIBRARIES += _pystorage.la
_pystorage_la_CPPFLAGS =						\
	$(AM_CPPFLAGS)							\
	-I$(top_srcdir)/src/nox						\
	-I$(top_srcdir)/src/nox/coreapps/				\
	-I$(srcdir)/../
nodist__pystorage_la_SOURCES = pystorage_wrap.cc
_pystorage_la_LDFLAGS = -module


NOX_RUNTIMEFILES +=							\
	__init__.py							\
	storage.py							\
	util.py		

NOX_PYBUILDFILES =							\
	pystorage.py							

NOX_PYLIBFILES = 							\
	storage-common.so						\
	storage-backend.so						\
	_pystorage.so							

endif # PY_ENABLED

all-local: nox-all-local 
clean-local: nox-clean-local
install-exec-hook: nox-install-local
