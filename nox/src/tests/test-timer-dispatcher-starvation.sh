#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-timer-dispatcher-starvation > tmp$$
diff -u - tmp$$ <<EOF
Timer 1 fired
Posting timer 2
Polled My_pollable
Timer 2 fired
Posting timer 3
Polled My_pollable
Timer 3 fired
Posting timer 4
Polled My_pollable
Timer 4 fired
Posting timer 5
Polled My_pollable
Timer 5 fired
EOF
