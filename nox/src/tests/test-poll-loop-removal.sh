#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-poll-loop-removal > tmp$$
diff -u - tmp$$ <<EOF
Polled My_pollable(1)
Polled My_pollable(2)
Polled My_pollable(2)
Polled My_pollable(2)
EOF
