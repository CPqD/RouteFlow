#!/bin/sh

RYU_GIT="git@github.com:joestringer/ryu-rfproxy.git"
RYU_BRANCH="origin/master"
RYU_DEPS="python-pip"

get_ryu() {
    # TODO: This recommendation should be revisited after the ovs-1.10+ release
    if [ "$OVS_VERSION" != "git" ]; then
        print_status "We recommend using ovs-git with Ryu for OF1.2+ Support" \
            $YELLOW
    fi

    print_status "Fetching Ryu controller"
    pkg_install "$RYU_DEPS"

    if [ $FETCH_ONLY -ne 1 ]; then
        $DO pip install ryu || fail "Failed to fetch ryu controller"
    fi

    fetch "ryu-" "rfproxy" $RYU_GIT $RYU_BRANCH ||
        fail "Couldn't fetch ryu-rfproxy"
    $DO cd -
}
