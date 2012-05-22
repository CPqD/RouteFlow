#!/bin/sh
ulimit unlimited
echo "+runtime: 8"
(cd "$abs_builddir/src" && \
./nox_core test_storage_table_util 2>&1 | tee $abs_testdir/test.out
)
if test $? = 0; then
    echo "+assert_pass: test_storage_table_util" 
else
    echo "+assert_fail: test_storage_table_util" 
fi    