# strerrorname_np.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRERRORNAME_NP],
[
  dnl Persuade glibc <string.h> to declare strerrorname_np().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_FUNCS([strerrorname_np])
  if test $ac_cv_func_strerrorname_np = yes; then
    dnl In glibc 2.32, strerrorname_np returns English error descriptions, not
    dnl error names.
    dnl See <https://sourceware.org/bugzilla/show_bug.cgi?id=26555>.
    AC_CACHE_CHECK([whether strerrorname_np works],
      [gl_cv_func_strerrorname_np_works],
      [AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <errno.h>
              #include <string.h>
            ]],
            [[return strcmp (strerrorname_np (EINVAL), "EINVAL") != 0;
            ]])],
         [gl_cv_func_strerrorname_np_works=yes],
         [gl_cv_func_strerrorname_np_works=no],
         [case "$host_os" in
            # Guess no on glibc systems.
            *-gnu* | gnu*)
              gl_cv_func_strerrorname_np_works="guessing no" ;;
            # Otherwise obey --enable-cross-guesses.
            *)
              gl_cv_func_strerrorname_np_works="$gl_cross_guess_normal" ;;
          esac
         ])
      ])
    case "$gl_cv_func_strerrorname_np_works" in
      *yes) ;;
      *) REPLACE_STRERRORNAME_NP=1 ;;
    esac
  else
    HAVE_STRERRORNAME_NP=0
  fi
])
