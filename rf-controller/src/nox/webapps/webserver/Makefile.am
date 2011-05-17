include ../../../Make.vars

EXTRA_DIST=\
	__init__.py \
	meta.json \
	webserver.py \
	webauth.py \
	static_www \
	dummywebpage.py

# Required for static_www build/install
MODULE=webserver
MODULE_BASE_PATH=nox/webapps
MODULE_BASE_ESCAPE=../..

NOX_RUNTIMEFILES = \
	__init__.py \
	meta.json \
    webserver.py \
	webauth.py \
	dummywebpage.py

# Executable scripts with the name "wwwroot-finalization-script.sh"
# found anywhere in the build tree will be run to make final changes
# to the www static file directory before install.
#
# Each script is called with two arguments.  The first is the path to
# the directory tree of wwwroot files to be installed.  This directory
# will only have a single subdirectory, called .files.  The tree under
# this subdirectory will be served as static files versioned by the
# build number.  The second is the path of a directory in which the
# script can place temporary files, etc.  The script does not have to
# worry about cleaning up files in that directory if something goes
# wrong in execution, etc.

WWW_INSTALL_FINAL_SUBDIR=$(builddir)/www-install-final
WWW_INSTALL_TMP_SUBDIR=$(builddir)/www-install-tmp

all-local: nox-runtimefiles-build static-www-build
	mkdir -p $(WWW_BUILD_SUBDIR)
	(cd $(WWW_BUILD_SUBDIR) && rm -f $(BUILDNR) && mkdir -p .files && ln -s .files $(BUILDNR))

clean-local: nox-runtimefiles-clean
	rm -rf "$(WWW_BUILD_SUBDIR)" "$(WWW_INSTALL_TMP_SUBDIR)" "$(WWW_INSTALL_TMP_SUBDIR)"

# Need to add test-wwwroot as a dependency to ensure it runs during builds
install-exec-hook : nox-install-local
	@echo "Copying files to prepare for wwwroot install"
	rm -rf "$(WWW_INSTALL_FINAL_SUBDIR)"
	mkdir -p "$(WWW_INSTALL_FINAL_SUBDIR)"
	cp -rLlf "$(WWW_BUILD_SUBDIR)/.files" "$(WWW_INSTALL_FINAL_SUBDIR)/"
	@echo "Executing scripts modifying wwwroot"
	@for s in `find $(top_srcdir) -wholename $(top_srcdir)/build -prune -o \\( -name wwwroot-finalization-script\\*.sh -print \\)`; do \
		if test -x "$$s"; then \
			rm -rf "$(WWW_INSTALL_TMP_SUBDIR)" \
			&& mkdir -p "$(WWW_INSTALL_TMP_SUBDIR)"; \
			echo "Running script: $$s"; \
			"$$s" "$(WWW_INSTALL_FINAL_SUBDIR)" "$(WWW_INSTALL_TMP_SUBDIR)"; \
		else \
			echo "Skipping script: $$s"; \
		fi; \
	done
	rm -rf "$(WWW_INSTALL_TMP_SUBDIR)"
	@echo "Copying modified wwwroot to install location"
	mkdir -p "$(nox_installwwwdir)"
	cp -r "$(WWW_INSTALL_FINAL_SUBDIR)/.files" "$(nox_installwwwdir)/$(BUILDNR)"

test-wwwroot:
	@echo "Executing scripts testing wwwroot"
	@for s in `find $(top_srcdir) -wholename $(top_srcdir)/build -prune -o \\( -name wwwroot-test-script\\*.sh -print \\)`; do \
		if test -x "$$s"; then \
			echo "Running script: $$s"; \
			"$$s" "$(WWW_BUILD_SUBDIR)"; \
		else \
			echo "Skipping script: $$s"; \
		fi; \
	done
