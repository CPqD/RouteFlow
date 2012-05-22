#!/bin/sh
ulimit unlimited
echo "+runtime: 4"
(cd "$abs_builddir/src" && \
./nox_core test_basic_callback 2>&1 | tee $abs_testdir/test.out
)
