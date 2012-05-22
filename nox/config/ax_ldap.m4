AC_DEFUN([CHECK_LDAP], [
AC_CHECK_HEADER([ldap.h],
                [HAVE_LDAP=yes],
                [HAVE_LDAP=no])
AM_CONDITIONAL([HAVE_LDAP], [test "$HAVE_LDAP" = yes])
if test "x$HAVE_LDAP" = "xyes"; then
   AC_DEFINE(HAVE_LDAP,1,[
Provide macro indicating the platform has ldap 
])
   LDAP_LDFLAGS="-lldap"
   AC_SUBST(LDAP_LDFLAGS)
fi
])
