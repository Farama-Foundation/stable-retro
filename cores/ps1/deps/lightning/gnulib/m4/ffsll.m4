# ffsll.m4 serial 2
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FFSLL],
[
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc <string.h> and AIX <strings.h> to declare ffsll().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([ffsll])
  if test $ac_cv_func_ffsll = yes; then
    dnl Test whether ffsll works.
    dnl On AIX 7.2 in 32-bit mode it is completely broken.
    AC_CACHE_CHECK([whether ffsll works],
      [gl_cv_func_ffsll_works],
      [AC_RUN_IFELSE(
         [AC_LANG_SOURCE([[
            #include <string.h>
            #include <strings.h>
            int dummy (long long x) { return 42; }
            int main (int argc, char *argv[])
            {
              int (* volatile my_ffsll) (long long) = argc ? ffsll : dummy;
              long long int x = -128LL;
              return my_ffsll (x) != 8;
            }
          ]])],
         [gl_cv_func_ffsll_works=yes],
         [gl_cv_func_ffsll_works=no],
         [case "$host_os" in
                           # Guess yes on glibc systems.
            *-gnu* | gnu*) gl_cv_func_ffsll_works="guessing yes" ;;
                           # Guess yes on musl systems.
            *-musl*)       gl_cv_func_ffsll_works="guessing yes" ;;
                           # Guess yes on native Windows.
            mingw*)        gl_cv_func_ffsll_works="guessing yes" ;;
                           # Guess no on AIX.
            aix*)          gl_cv_func_ffsll_works="guessing no" ;;
                           # If we don't know, obey --enable-cross-guesses.
            *)             gl_cv_func_ffsll_works="$gl_cross_guess_normal" ;;
          esac
         ])
      ])
    case "$gl_cv_func_ffsll_works" in
      *yes) ;;
      *) REPLACE_FFSLL=1 ;;
    esac
  else
    HAVE_FFSLL=0
  fi
])
