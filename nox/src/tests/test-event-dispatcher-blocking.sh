#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-event-dispatcher-blocking > tmp$$
diff -u - tmp$$ <<EOF
Handling event 0
  Blocking...
Polled My_pollable
Handling event 1
  Blocking...
Polled My_pollable
Handling event 2
  Blocking...
Polled My_pollable
Handling event 3
  Waking event 0 blocker
  Posting event 6
Handling event 4
  Waking event 1 blocker
  Posting event 7
Handling event 5
  Waking event 2 blocker
  Posting event 8
Event 0 blocker woke up...
Event 1 blocker woke up...
Event 2 blocker woke up...
Polled My_pollable
Handling event 6
Handling event 7
Handling event 8
EOF
