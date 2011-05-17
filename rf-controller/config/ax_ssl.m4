AC_DEFUN([CHECK_SSL], [
AC_CHECK_HEADER([openssl/md5.h],
     ,
    AC_ERROR([openssl/md5.h not found. NOX requires OpenSSL])
)
AC_CHECK_LIB(ssl, MD5_Init,
             [SSL_LIBS="-lssl"; AC_SUBST(SSL_LIBS) break],
             [AC_ERROR([openssl/md5.h not found. NOX requires OpenSSL])])
AC_CHECK_PROG(OPENSSL_PROG, openssl,
             [yes],
             [AC_ERROR([openssl program not found. NOX requires OpenSSL])])
])

AC_DEFUN([FIGURE_SSL_CONST], [
    AC_MSG_CHECKING(if SSL_CTX_new() can take a const argument)
    AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    @%:@include <openssl/ssl.h>
    ]], [[SSL_CTX_new((const SSL_METHOD*)0);]])],[
        AC_MSG_RESULT(yes)
        AC_DEFINE([SSL_CONST],
          [const],[Define to 'const' if SSL_CTX_new() takes a const argument])
      ],[
        AC_MSG_RESULT(no)
        AC_DEFINE([SSL_CONST],
          [],[Define to 'const' if SSL_CTX_new() takes a const argument])
        ])
    AC_LANG_POP([C++])
])
