AUTOMAKE_OPTIONS = foreign 1.10
ACLOCAL_AMFLAGS = -I config

SUBDIRS = \
	src \
	$(subdirs) \
	doc \
	man

dist-local:
	cd doc/manual && make clean

nox_installsubdir = $(DESTDIR)$(subst src,$(bindir),$(subdir))

install-exec-hook:
	(cd $(DESTDIR)$(bindir) && $(PYTHON) -c "import compileall; compileall.compile_dir('.', force=True)") 
	(cd $(DESTDIR)$(bindir) && find . -name "*.py" -exec rm -f {} \;)

test:
	@$(PYTHON) src/nox/coreapps/testharness/testharness.py -b $(top_builddir) $(top_builddir)/src

clean-local:
	rm -rf __nox_tests
