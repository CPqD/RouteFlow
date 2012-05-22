#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-coop-signals > tmp$$
diff -u - tmp$$ <<EOF
down
up
try_dequeue
dequeue
kill
join
EOF
