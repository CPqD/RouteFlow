#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-coop-preblock-hook > tmp$$
diff -u - tmp$$ <<EOF
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
thread1
hook
thread2
EOF
