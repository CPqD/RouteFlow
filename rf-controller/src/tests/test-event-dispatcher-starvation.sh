#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-event-dispatcher-starvation > tmp$$
diff -u - tmp$$ <<EOF
Handling event 1
  Handler posting event 6
Handling event 2
  Handler posting event 7
Polled My_pollable
  My_pollable posting event 11
Handling event 6
Handling event 7
Handling event 11
Polled My_pollable
  My_pollable posting event 12
Handling event 12
Polled My_pollable
  My_pollable posting event 13
Handling event 13
Polled My_pollable
  My_pollable posting event 14
Handling event 14
EOF
