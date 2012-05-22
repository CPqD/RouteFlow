#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-timer-dispatcher-duplicates > tmp$$
diff -u - tmp$$ <<EOF
Timer 0 fired
Timer 1 fired
Timer 2 fired
Timer 3 fired
Timer 4 fired
EOF
