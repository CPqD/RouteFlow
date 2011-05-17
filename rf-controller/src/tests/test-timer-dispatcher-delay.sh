#! /bin/sh -e
trap 'rm -f tmp$$*' 0

$SUPERVISOR ./test-timer-dispatcher-delay > tmp$$.1

diff -u - tmp$$.1 > tmp$$.2 <<EOF && code=$? || code=$?
Timer 1 fired
Polled My_pollable
Timer 4 fired
Polled My_pollable
Timer 7 fired
Polled My_pollable
Timer 10 fired
Polled My_pollable
Timer 13 fired
Polled My_pollable
Timer 16 fired
Timer 17 fired
Timer 18 fired
EOF

if test "$code" = 0; then
    # No differences.  OK.
    exit 0
elif test "$code" = 1; then
    # Some differences.  If the only differences are additional polls of
    # My_pollable, which indicates that the timers took a little extra
    # time to expire, then OK.
    if grep '^[-+]' tmp$$.2 | egrep -v '\+\+\+|---|\+Polled My_pollable'; then
        cat tmp$$.2
        exit 1
    fi
    exit 0
else
    exit 1
fi
