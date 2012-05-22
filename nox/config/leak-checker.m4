dnl Checks for __malloc_hook, etc., supported by glibc.
AC_DEFUN([CHECK_MALLOC_HOOKS],
  [AC_CACHE_CHECK(
    [whether libc supports hooks for malloc and related functions],
    [ofp_cv_malloc_hooks],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM(
         [#include <malloc.h>
         ], 
         [(void) __malloc_hook;
          (void) __realloc_hook;
          (void) __free_hook;])],
      [ofp_cv_malloc_hooks=yes],
      [ofp_cv_malloc_hooks=no])])
   if test $ofp_cv_malloc_hooks = yes; then
     AC_DEFINE([HAVE_MALLOC_HOOKS], [1], 
               [Define to 1 if you have __malloc_hook, __realloc_hook, and
                __free_hook in <malloc.h>.])
   fi])

