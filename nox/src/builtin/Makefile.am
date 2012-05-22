include ../Make.vars


pkglib_LTLIBRARIES = libbuiltin.la

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED
AM_CPPFLAGS += $(OPENFLOW_CPPFLAGS)
AM_CPPFLAGS += -I$(top_srcdir)/src/nox


libbuiltin_la_SOURCES = \
	deployer.cc			\
	component.cc			\
	dso-deployer.cc			\
	event-dispatcher-component.cc	\
	event-dispatcher-component.hh	\
	kernel.cc			\
	nox.cc				\
	packet-classifier.cc		\
	static-deployer.cc
