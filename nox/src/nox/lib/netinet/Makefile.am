include ../../../Make.vars 

EXTRA_DIST =\
	__init__.py

NOX_RUNTIMEFILES = 
NOX_PYBUILDFILES = 
NOX_PYLIBFILES = 

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)

netinet_wrap_includes = 			\
	ethernetaddr.i				\
	ipaddr.i				\
	cidr.i					\
	netinet.i				\
	datapathid.i        			\
	packet-expr.i				\
	flow.i

netinet_wrap.cc netinet.py: $(netinet_wrap_includes)
	$(SWIG) $(SWIG_PYTHON_OPT) -outdir $(builddir)/. -I$(top_srcdir)/src/nox/lib/ -o netinet_wrap.cc -module netinet $(srcdir)/netinet.i

pkglib_LTLIBRARIES = _netinet.la
_netinet_la_SOURCES = netinet.i  $(netinet_wrap_includes)
nodist__netinet_la_SOURCES = netinet_wrap.cc
_netinet_la_LDFLAGS = -module
_netinet_la_CPPFLAGS = $(AM_CPPFLAGS)

NOX_RUNTIMEFILES += \
	__init__.py

NOX_PYBUILDFILES += \
	netinet.py

NOX_PYLIBFILES += \
	_netinet.so

endif # PY_ENABLED

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
