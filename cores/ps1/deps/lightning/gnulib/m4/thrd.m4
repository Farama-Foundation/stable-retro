# thrd.m4 serial 1
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_THRD_JOIN],
[
  AC_REQUIRE([gl_THREADS_H])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  if test $ac_cv_header_threads_h = yes; then
    if test $BROKEN_THRD_START_T = 1; then
      REPLACE_THRD_CREATE=1
      REPLACE_THRD_CURRENT=1
      REPLACE_THRD_DETACH=1
      REPLACE_THRD_EQUAL=1
      REPLACE_THRD_JOIN=1
      AC_DEFINE([BROKEN_THRD_START_T], [1],
        [Define if the thrd_start_t type is not as described in ISO C 11.])
    fi

    dnl On Solaris 11.4, thrd_join crashes when the second argument is NULL.
    AC_CACHE_CHECK([whether thrd_join works],
      [gl_cv_func_thrd_join_works],
      [save_LIBS="$LIBS"
       LIBS="$LIBS $LIBSTDTHREAD"
       AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <stddef.h>
              #include <threads.h>
              static int func (void *arg)
              {
                return (arg != NULL);
              }
            ]],
            [[thrd_t thread;
              if (thrd_create (&thread, func, NULL) != thrd_success)
                return 1;
              if (thrd_join (thread, NULL) != thrd_success)
                return 2;
              return 0;
            ]])],
         [gl_cv_func_thrd_join_works=yes],
         [gl_cv_func_thrd_join_works=no],
         [case "$host_os" in
                      # Only Solaris is known to be broken.
            solaris*) gl_cv_func_thrd_join_works="guessing no" ;;
            *)        gl_cv_func_thrd_join_works="guessing yes" ;;
          esac
         ])
       LIBS="$save_LIBS"
      ])
    case "$gl_cv_func_thrd_join_works" in
      *yes) ;;
      *)
        REPLACE_THRD_JOIN=1
        AC_DEFINE([BROKEN_THRD_JOIN], [1],
          [Define if the thrd_join function does not behave as in ISO C 11.])
        ;;
    esac
  fi
])
