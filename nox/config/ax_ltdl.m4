AC_DEFUN([CHECK_LTDL], [
AC_ARG_ENABLE(
  [ltdl],
  [AC_HELP_STRING([--enable-ltdl],
                  [Use libtool for runtime dynamic library loading])],
  [case "${enableval}" in # (
     yes) ltdl=true ;; # (
     no)  ltdl=false ;; # (
     *) AC_MSG_ERROR([bad value ${enableval} for --enable-ltdl]) ;;
   esac],
  [ltdl=false])

  AM_CONDITIONAL([USE_LTDL], [test "x$ltdl" = "xtrue"])

  if test "x$ltdl" = "xtrue"; then
      AC_DEFINE(USE_LTDL,1,[
Provide macro indicating the preference to use ltdl
])
  fi
])
