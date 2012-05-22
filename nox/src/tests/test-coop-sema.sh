#! /bin/sh -e
trap 'rm -f tmp$$' 0
$SUPERVISOR ./test-coop-sema > tmp$$
diff -u - tmp$$ <<EOF
Up without yield
up
up
up
up
up
up
up
up
up
up
down
down
down
down
down
down
down
down
down
down

Up with yield
up
down
up
down
up
down
up
down
up
down
up
down
up
down
up
down
up
down
up
down

Alternating wakers
up sema1
block wake1=1 wake2=0
up sema2
block wake1=0 wake2=1
up sema1
block wake1=1 wake2=0
up sema2
block wake1=0 wake2=1
up sema1
block wake1=1 wake2=0
up sema2
block wake1=0 wake2=1
up sema1
block wake1=1 wake2=0
up sema2
block wake1=0 wake2=1
up sema1
block wake1=1 wake2=0
up sema2
block wake1=0 wake2=1

FIFO queuing
start id=0
start id=1
start id=2
start id=3
start id=4
start id=5
start id=6
start id=7
start id=8
start id=9
up id=0
wake id=0
up id=1
wake id=1
up id=2
wake id=2
up id=3
wake id=3
up id=4
wake id=4
up id=5
wake id=5
up id=6
wake id=6
up id=7
wake id=7
up id=8
wake id=8
up id=9
wake id=9
EOF
