#!/bin/sh
ulimit unlimited
echo "+runtime: 4"
(cd "$abs_builddir/src" && \
./nox_core test_bs_link 2>&1 | tee $abs_testdir/test.out
)
if test $? = 0; then
    echo "+assert_pass: test_bs_link" 
else
    echo "+assert_fail: test_bs_link" 
fi    
