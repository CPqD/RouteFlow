include ../../Make.vars 

SUBDIRS = netinet packet
EXTRA_DIST =\
	openflow.i\
	config.i\
	buffer.i\
	common-defs.i\
	core.py\
	__init__.py\
	util.py\
	token_bucket.py\
	directory.py\
	directory_factory.py\
	registries.py\
	utf8_string.i

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)

pkglib_LTLIBRARIES = \
   _openflow.la \
   _config.la

openflow_wrap.cc openflow.py: openflow.i $(OPENFLOW_HEADER_FILE)
	$(SWIG) $(SWIG_PYTHON_OPT) -outdir $(builddir)/. $(OPENFLOW_CPPFLAGS) -o openflow_wrap.cc -module openflow $(srcdir)/openflow.i
_openflow_la_SOURCES = nox-config.hh 
nodist__openflow_la_SOURCES = openflow_wrap.cc
_openflow_la_LDFLAGS = -module
_openflow_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox

config_wrap.cc config.py: config.i
	$(SWIG) $(SWIG_PYTHON_OPT) -outdir $(builddir)/. -o config_wrap.cc -module config $(srcdir)/config.i
_config_la_SOURCES = nox-config.hh 
nodist__config_la_SOURCES = config_wrap.cc
_config_la_LDFLAGS = -module
_config_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox

NOX_RUNTIMEFILES = \
	core.py\
	__init__.py\
	util.py\
	token_bucket.py\
	directory.py\
	directory_factory.py\
	registries.py\
	utf8_string.i

NOX_PYBUILDFILES = \
	openflow.py \
	config.py

NOX_PYLIBFILES = \
   _openflow.so\
   _config.so


endif # PY_ENABLED

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
