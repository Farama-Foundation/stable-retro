# wcstok.m4 serial 6
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCSTOK],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CHECK_FUNCS_ONCE([wcstok])
  if test $ac_cv_func_wcstok = yes; then
    dnl On HP-UX 11.31, it depends on the compiler and linker whether wcstok()
    dnl uses hidden state, ignoring the third argument, and thus exhibits a
    dnl bug when two or more wcstok() iteration loops are being performed in
    dnl the same thread.
    case "$host_os" in
      hpux*)
        REPLACE_WCSTOK=1
        ;;
      *)
        dnl POSIX:        wchar_t *wcstok (wchar_t *, const wchar_t *, wchar_t **);
        dnl mingw, MSVC:  wchar_t *wcstok (wchar_t *, const wchar_t *);
        AC_CACHE_CHECK([for wcstok with POSIX signature],
          [gl_cv_func_wcstok_posix_signature],
          [AC_COMPILE_IFELSE(
             [AC_LANG_PROGRAM(
                [[#include <wchar.h>
                  wchar_t *wcstok (wchar_t *, const wchar_t *, wchar_t **);
                ]],
                [[]])],
             [gl_cv_func_wcstok_posix_signature=yes],
             [gl_cv_func_wcstok_posix_signature=no])
          ])
        if test $gl_cv_func_wcstok_posix_signature = no; then
          REPLACE_WCSTOK=1
        fi
        ;;
    esac
  else
    HAVE_WCSTOK=0
  fi
])
