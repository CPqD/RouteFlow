AC_DEFUN([CHECK_PCAP], [
AC_CHECK_HEADER([pcap.h],
                [HAVE_PCAP=yes],
                [HAVE_PCAP=no])
AM_CONDITIONAL([HAVE_PCAP], [test "$HAVE_PCAP" = yes])
if test "x$HAVE_PCAP" = "xyes"; then
   AC_DEFINE(HAVE_PCAP,1,[
Provide macro indicating the platform has pcap 
])
   PCAP_LDFLAGS="-lpcap"
   AC_SUBST(PCAP_LDFLAGS)
fi
])
