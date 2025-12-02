# aligned_alloc.m4 serial 3
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ALIGNED_ALLOC],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc and OpenBSD <stdlib.h> to declare aligned_alloc().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([aligned_alloc])
  if test $ac_cv_func_aligned_alloc = yes; then
    dnl On macOS 11.1 and AIX 7.2, aligned_alloc returns NULL when the alignment
    dnl argument is smaller than sizeof (void *).
    AC_CACHE_CHECK([whether aligned_alloc works for small alignments],
      [gl_cv_func_aligned_alloc_works],
      [AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <stdlib.h>
            ]],
            [[return aligned_alloc (2, 18) == NULL;
            ]])
         ],
         [gl_cv_func_aligned_alloc_works=yes],
         [gl_cv_func_aligned_alloc_works=no],
         [case "$host_os" in
                     # Guess no on AIX.
            aix*)    gl_cv_func_aligned_alloc_works="guessing no" ;;
                     # Guess no on macOS.
            darwin*) gl_cv_func_aligned_alloc_works="guessing no" ;;
                     # If we don't know, obey --enable-cross-guesses.
            *)       gl_cv_func_aligned_alloc_works="$gl_cross_guess_normal" ;;
          esac
         ])
      ])
    case "$gl_cv_func_aligned_alloc_works" in
      *yes) ;;
      *) REPLACE_ALIGNED_ALLOC=1 ;;
    esac
  else
    dnl The system does not have aligned_alloc.
    HAVE_ALIGNED_ALLOC=0
  fi
])
