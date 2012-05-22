#!/bin/sh
echo "+runtime: 20"
(cd "$abs_builddir/src" && \
./nox_core test_example 
)
if test $? = 0; then
    echo "+assert_pass: test_example" 
else
    echo "+assert_fail: test_example" 
fi    
