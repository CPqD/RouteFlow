AC_DEFUN([AC_PROG_SPHINX],[
        AC_PATH_PROG([SPHINX_BUILD],[sphinx-build])
        if test -z "$SPHINX_BUILD" ; then
                AC_MSG_WARN([cannot find sphinx-build. While not build documentation])
        fi
        AM_CONDITIONAL([HAVE_SPHINX_BUILD], [test -n "$SPHINX_BUILD"])
])
