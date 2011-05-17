include ../../../Make.vars 

EXTRA_DIST = \
	meta.json \
	__init__.py \
	test_packet.py \
	servercert.pem \
	cacert.pem \
	serverkey.pem \
    test_basic_callback.py \
    test_mod.py \
	test_packet.sh  \
    test_basic_callback.sh \
	test_async.sh \
	test_ssl.sh \
	ssl-test-str.hh \
	TEST_DIRECTORY

pkglib_LTLIBRARIES =		\
	ssl_test.la \
    async_test.la 

ssl_test_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/coreapps/
ssl_test_la_SOURCES  = ssl-test.cc ssl-test.hh
ssl_test_la_LDFLAGS  = -module -export-dynamic

async_test_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/coreapps/
async_test_la_SOURCES  = async-test.cc async-test.hh
async_test_la_LDFLAGS  = -module -export-dynamic

NOX_RUNTIMEFILES = \
    meta.json \
	__init__.py \
	test_packet.py \
	servercert.pem \
	serverkey.pem \
	cacert.pem \
    test_mod.py \
    test_basic_callback.py \
	test_packet.sh \
    test_basic_callback.sh \
	test_async.sh \
	test_ssl.sh \
	TEST_DIRECTORY

all-local: nox-all-local
clean-local: nox-clean-local
install-exec-hook: nox-install-local
