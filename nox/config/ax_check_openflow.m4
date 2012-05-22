# -*- autoconf -*-

dnl Checks for openflow header files.
AC_DEFUN([CHECK_OPENFLOW],
  [AC_ARG_WITH(
     [openflow],
     [AC_HELP_STRING([--with-openflow=/path/to/openflow/],
                     [Specify absolute path to openflow directory])],
     [path="$withval"], 
     [path="no"])
   if test -n "$path" && test "$path" != "no"; then
     if ! test -e "$path/include/openflow/openflow.h"; then
       AC_MSG_ERROR([$path/include/openflow/openflow.h does not exist])
     fi
     if ! test -e "$path/include/openflow/nicira-ext.h"; then
       AC_MSG_ERROR([$path/include/openflow/nicira-ext.h does not exist])
     fi
     if ! test -e "$path/include/openflow/openflow-netlink.h"; then
       AC_MSG_ERROR([$path/include/openflow/openflow-netlink.h does not exist])
     fi
     OPENFLOW_CPPFLAGS="-I$path/include"
   else
     OPENFLOW_CPPFLAGS='-I${top_srcdir}/src/include/openflow'
   fi    
   AC_SUBST([OPENFLOW_CPPFLAGS])
])
