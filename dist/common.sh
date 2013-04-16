#!/bin/sh

COLOUR_CHECKED=0
DELIM="\033["
ECHO_OPTS=""

REVERT="0m"
RED="31m"
GREEN="32m"
YELLOW="33m"

colour_check() {
    if [ $COLOUR_CHECKED -eq 0 ]; then
        COLOUR_CHECKED=1
        if (! echo -e "${DELIM}${RED}COLOUR${DELIM}0m" | grep -q "e"); then
            COLOUR="-e"
        elif (echo "${DELIM}${RED}COLOUR${DELIM}0m" | grep -q "033"); then
            DELIM=""
            REVERT=""
        else
            COLOUR=""
        fi
    fi
}

highlight() {
    local_colour="$2"
    if [ "$DELIM" = "" ]; then
        local_colour=""
    fi

    colour_check
    echo $COLOUR "${DELIM}${2}${1}${DELIM}${REVERT}"
}

fail() {
    highlight "$1" "$RED"
    exit 1
}

print_status() {
    if [ $# -eq 2 ]; then
        highlight "${1}..." "$2"
    else
        highlight "${1}..." "$GREEN"
    fi
}

##
# Fetch the source for a project
#
# $1 - file name, minus the version
# $2 - version number (or "git" or "rfproxy")
# $3 - git URL to fetch source from
# $4 - (for git/rfproxy) remote git branch to synchronise with
# $5 - base URL to fetch non-git source package from (full path=$4$1$2.tar.gz)
##
fetch() {
    NAME="$1$2"

    print_status "Getting $NAME"
    if [ "$2" = "git" ] || [ "$2" = "rfproxy" ]; then
        if [ ! -e $NAME ]; then
            $DO git clone $3 $NAME || return 1
        fi
        $DO cd $NAME
        if [ $UPDATE -eq 1 ]; then
            $DO git fetch || return 1
            $DO git checkout $4 || return 1
        fi
        if [ $FETCH_ONLY -eq 1 ]; then
            $DO cd -
        fi
    elif [ ! -e $NAME ]; then
        if [ ! -e $NAME.tar.gz ]; then
            $DO wget ${5}/${NAME}.tar.gz || return 1
        fi
        if [ $FETCH_ONLY -ne 1 ]; then
            $DO tar xzf ${NAME}.tar.gz || return 1
            $DO cd $NAME || return 1
        fi
    fi

    return 0
}

pkg_install() {
    $SUPER apt-get $APT_OPTS install $@ ||
        fail "Couldn't install packages"
}
