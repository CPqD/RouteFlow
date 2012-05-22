dnl --
dnl CHECK_TWISTED
dnl
dnl Enable use of Twisted python 
dnl --
AC_DEFUN([CHECK_TWISTED], [
  AC_ARG_WITH([python],
              [AC_HELP_STRING([--with-python=/path/to/python.binary|yes|no],
                              [Specify python binary (must be v2.5 or greater and must have twisted installed)])],
               [path="$withval"], [path="yes"])dnl

  if test "x$path" != "xno"; then
    if test -n "$path"; then
        PYTHON="$path"
    fi    

    AC_PYTHON_DEVEL([>='2.5'])

    AC_MSG_CHECKING([whether twisted python is installed])
    `$PYTHON -c "import twisted" 2> /dev/null`
    RETVAL=$?
    if (( $RETVAL != 0 )); then
        AC_MSG_RESULT([no])
        AC_MSG_WARN([twisted not installed, compiling without Python support])
        PYTHON=""
    else
        AC_MSG_RESULT([yes])
    fi

  fi

  if test -n "$PYTHON"; then
    AC_DEFINE(TWISTED_ENABLED,1,[
Provide macro indicating that twisted python was enabled
])
  fi
  AM_CONDITIONAL(PY_ENABLED, test -n "$PYTHON")
])
