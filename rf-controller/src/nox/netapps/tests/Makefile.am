include ../../../Make.vars

SUBDIRS = pyunittests

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED

pkglib_LTLIBRARIES = tests.la

tests_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(srcdir)/..			\
	-I $(top_builddir)/src/nox/ -I $(builddir)/..			\
	-I$(top_srcdir)/src/nox						\
	-I$(top_srcdir)/src/nox/coreapps 				\
	-I$(top_builddir)/src		 				\
	-I$(top_builddir)/src/nox/netapps 				\
	-I$(top_srcdir)/src/nox/thirdparty/protobuf/src			\
	-D__COMPONENT_FACTORY_FUNCTION__=tests_get_factory
tests_la_CFLAGS = $(AM_CFLAGS)
tests_la_LDFLAGS = -module -export-dynamic
tests_la_SOURCES = 							\
	async-test.cc							\
	ssl-test-str.hh							\
	ssl-test.cc							\
	tests.cc							\
	tests.hh

# TODO: figure out the destiny of these...
#	tests/file-server.cc			
#	tests/file-server.hh			

EXTRA_DIST = 								\
	meta.json                    					\
	__init__.py                 					\
	servercert.pem							\
	cacert.pem							\
	serverkey.pem							\
	pytests.hh							\
	unittest.py							\
	pytests.i     

NOX_RUNTIMEFILES = 							\
	meta.json							\
	servercert.pem							\
	serverkey.pem							\
	cacert.pem		

if PY_ENABLED
NOX_PYLIBFILES = _pytests.so

NOX_RUNTIMEFILES += 							\
	unittest.py							\
	__init__.py

NOX_PYBUILDFILES =							\
	pytests.py

pytests_wrap.cc pytests.py: pytests.i
	$(SWIG) $(SWIG_PYTHON_OPT) -outdir $(builddir)/. -o pytests_wrap.cc \
	-module pytests $(srcdir)/pytests.i
pkglib_LTLIBRARIES += _pytests.la
nodist__pytests_la_SOURCES = pytests_wrap.cc
_pytests_la_LDFLAGS = -module

endif # PY_ENABLED

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
