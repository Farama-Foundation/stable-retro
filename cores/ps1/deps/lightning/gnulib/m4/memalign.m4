# memalign.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMALIGN],
[
  AC_REQUIRE([gl_MALLOC_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  AC_CHECK_FUNCS_ONCE([memalign])
  if test $ac_cv_func_memalign = yes; then
    dnl On Solaris 11, memalign (2, n) always returns NULL.
    AC_CACHE_CHECK([whether memalign works for small alignments],
      [gl_cv_func_memalign_works],
      [AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <malloc.h>
              #include <stdlib.h>
            ]],
            [[int result = 0;
              if (memalign (1, 1) == NULL)
                result |= 1;
              if (memalign (2, 1) == NULL)
                result |= 2;
              return result;
            ]])
         ],
         [gl_cv_func_memalign_works=yes],
         [gl_cv_func_memalign_works=no],
         [case "$host_os" in
                      # Guess no on Solaris.
            solaris*) gl_cv_func_memalign_works="guessing no" ;;
                      # If we don't know, obey --enable-cross-guesses.
            *)        gl_cv_func_memalign_works="$gl_cross_guess_normal" ;;
          esac
         ])
      ])
    case "$gl_cv_func_memalign_works" in
      *yes) ;;
      *) REPLACE_MEMALIGN=1 ;;
    esac
  else
    dnl The system does not have memalign.
    HAVE_MEMALIGN=0
  fi
])
