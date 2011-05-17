AC_DEFUN([AC_PROG_DOXYGEN],[
        AC_PATH_PROG([DOXYGEN],[doxygen])
        if test -z "$DOXYGEN" ; then
                AC_MSG_WARN([cannot find doxygen. Will not build source documentation])
        fi
        AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
])
