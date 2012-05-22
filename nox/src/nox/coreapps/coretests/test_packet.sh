#!/bin/sh
ulimit unlimited
echo "+runtime: 15"
(cd "$abs_builddir/src" && \
./nox_core test_packet
)
