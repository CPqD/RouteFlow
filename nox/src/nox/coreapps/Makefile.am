include ../../Make.vars 

SUBDIRS = 
SUBDIRS      += $(MODULES_coreapps)

EXTRA_DIST = \
	__init__.py

if PY_ENABLED
AM_CPPFLAGS += -I $(INCLUDEPY)

NOX_RUNTIMEFILES = __init__.py

endif # PY_ENABLED

all-local:  nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
