AC_DEFUN([CHECK_RTC], [
AC_CHECK_HEADER([linux/rtc.h], [HAVE_RTC=yes], [HAVE_RTC=no])
AM_CONDITIONAL([HAVE_RTC], [test "$HAVE_RTC" = yes])
])
