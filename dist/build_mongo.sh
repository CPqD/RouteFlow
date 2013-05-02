#!/bin/sh

MONGO_URL="http://downloads.mongodb.org/src"
MONGO_GIT="git@github.com:mongodb/mongo.git"
MONGO_BRANCH="origin/master"

MONGO_BINARY="mongodb"
MONGO_DEPS="scons"

NS_FIX="a1e68969d48bbb47c893870f6428048a602faf90.patch"

##
# Fetch and build MongoDB from source.
#
# $1 - Version string or "git"
##
build_mongo() {
    if [ -e "mongodb-src-r$1" ] && [ $UPDATE -eq 0 ]; then
        return;
    fi

    fetch "mongodb-src-r" $1 $MONGO_GIT $MONGO_BRANCH $MONGO_URL ||
        fail "Couldn't fetch MongoDB"
    pkg_install "$MONGO_DEPS"

    # If we're running MongoDB < 2.4.0, there is a namespace conflict that
    # we need to patch for.
    verlt $1 "2.4.0" && if [ ! -e "${NS_FIX}" ]; then
        print_status "Patching MongoDB"
        $DO cp ${RFDIR}/dist/${NS_FIX} .
        $DO patch -p1 -i ${NS_FIX} || fail "Couldn't apply MongoDB patch"
    fi

    if [ $FETCH_ONLY -ne 1 ]; then
        print_status "Building MongoDB"
        $DO scons all || fail "Couldn't build MongoDB"
        print_status "Installing MongoDB"
        $SUPER scons --full install --prefix=/usr --sharedclient ||
            fail "Couldn't install MongoDB"
        $DO cd -
    fi
}

get_mongo() {
    if [ "$1" = "deb" ]; then
        pkg_install "$MONGO_BINARY"
    else
        build_mongo $@
    fi
}
