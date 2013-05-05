#!/bin/sh

OVS_URL="http://openvswitch.org/releases"
OVS_GIT="git://openvswitch.org/openvswitch"
OVS_BRANCH="origin/master"

OVS_BINARY="openvswitch-switch openvswitch-datapath-source \
    linux-headers-generic module-assistant"

install_ovs() {
    print_status "Installing Open vSwitch"

    $SUPER make install || return 1

    $SUPER mkdir /lib/modules/`uname -r`/kernel/net/ovs || return 1
    $SUPER cp datapath/linux/*.ko /lib/modules/`uname -r`/kernel/net/ovs/ ||
        return 1
    $SUPER depmod -a || return 1
    $SUPER modprobe openvswitch || return 1
    grep -q openvswitch /etc/modules

    status=$?
    if [ $status -eq 1 ]; then
        $SUPER echo "openvswitch" >>/etc/modules || return 1
    elif [ $status -ne 0 ]; then
        print_status "Can't add openvswitch_mod to /etc/modules" $YELLOW
    fi

    $SUPER mkdir -p /usr/local/etc/openvswitch || return 1
    $SUPER ovsdb-tool create /usr/local/etc/openvswitch/conf.db \
        vswitchd/vswitch.ovsschema || return 1

    return 0
}

##
# Fetch and build Open vSwitch from source.
#
# $1 - Version string or "git"
##
build_ovs() {
    if [ -e "openvswitch-$1" ] && [ $UPDATE -eq 0 ]; then
        return;
    fi

    fetch "openvswitch-" $1 $OVS_GIT $OVS_BRANCH $OVS_URL ||
        fail "Couldn't fetch OVS"

    if [ $FETCH_ONLY -ne 1 ]; then
        print_status "Building Open vSwitch"
        if [ ! -e "./configure" ]; then
            ./boot.sh || fail "Couldn't bootstrap Open vSwitch"
        fi
        if [ ! -e "Makefile" ]; then
            $DO ./configure --with-linux=/lib/modules/`uname -r`/build ||
                fail "Couldn't configure Open vSwitch"
        fi
        $DO make || fail "Couldn't build Open vSwitch"

        install_ovs || fail "Couldn't install Open vSwitch"
        $DO cd -
    fi
}

get_ovs() {
    if [ "$1" = "deb" ]; then
        pkg_install "$OVS_BINARY"
        $SUPER module-assistant auto-install openvswitch-datapath
    else
        build_ovs $@
    fi
}
