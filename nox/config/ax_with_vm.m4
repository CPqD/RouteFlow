AC_DEFUN([CHECK_WITH_VM], [
AC_ARG_WITH(vm,
  [AC_HELP_STRING([--with-vm],
                  [Directory containing hda.dsk for VM boot])],
  [VMDIR=`eval "echo $withval"`
   if test -e "${VMDIR}/hda.dsk"; then
     :
   else
     AC_MSG_ERROR([cannot find virtual disk $VMDIR/hda.dsk])
   fi])
AC_SUBST(VMDIR)
])
