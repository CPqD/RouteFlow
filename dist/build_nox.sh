#!/bin/sh

NOX_GIT="https://github.com/joestringer/nox-rfproxy.git"
NOX_BRANCH="origin/master"
NOX_DEPS="linux-headers-generic autoconf automake libtool \
    libboost-test-dev libssl-dev libpcap-dev python-twisted \
    python-simplejson python-dev swig1.3"

get_nox() {
    TWISTED="/usr/lib/python2.6/dist-packages/twisted/internet/base.py"

    if [ $FETCH_ONLY -ne 1 ]; then
        grep -q _handleSigchld $TWISTED
        status=$?

        if [ $status -eq 1 ]; then
            print_status "Patching Twisted on system"
            $DO cd /usr/lib/python2.6/dist-packages/twisted/internet/
            $SUPER patch -i ${RFDIR}/dist/twisted-internet-base.patch ||
                fail "Couldn't patch Twisted."
            $DO cd -
        elif [ $status -ne 0 ]; then
            print_status "Can't patch Twisted on system" $YELLOW
        fi
    fi

    fetch "nox-" "rfproxy" $NOX_GIT $NOX_BRANCH ||
        fail "Couldn't fetch nox-rfproxy"
    pkg_install "$NOX_DEPS"

    if [ $FETCH_ONLY -ne 1 ]; then
        print_status "Building NOX"

        if [ ! -e "./configure" ]; then
            $DO ./boot.sh
        fi
        $DO mkdir -p ../build/nox
        $DO cd ../build/nox
        if [ ! -e "Makefile" ]; then
            $DO ../../nox-rfproxy/configure --enable-ndebug ||
                fail "Couldn't configure NOX"
        fi

        $DO make || fail "Couldn't build NOX"
        $DO cd ../..
    fi
}
