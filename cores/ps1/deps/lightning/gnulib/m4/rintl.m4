# rintl.m4 serial 8
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RINTL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare rintl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine RINTL_LIBM.
  gl_MATHFUNC([rintl], [long double], [(long double)])
  if test $gl_cv_func_rintl_no_libm = yes \
     || test $gl_cv_func_rintl_in_libm = yes; then
    if test $REPLACE_RINTL = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether rintl works],
        [gl_cv_func_rintl_works],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $RINTL_LIBM"
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE([[
#include <math.h>
#undef rintl
extern
#ifdef __cplusplus
"C"
#endif
long double rintl (long double);
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_rintl) (long double) = argc ? rintl : dummy;
  int result = 0;
  /* This test fails on NetBSD 9.0.  */
  {
    volatile long double x = -0.3L;
    long double y = my_rintl (x);
    if (!(y == 0.0L))
      result |= 1;
  }
  return result;
}
            ]])],
            [gl_cv_func_rintl_works=yes],
            [gl_cv_func_rintl_works=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_rintl_works="guessing yes" ;;
                              # Guess yes on musl systems.
               *-musl*)       gl_cv_func_rintl_works="guessing yes" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_rintl_works="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_rintl_works="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_rintl_works" in
        *yes) ;;
        *) REPLACE_RINTL=1 ;;
      esac
    fi
  else
    HAVE_RINTL=0
  fi
  if test $HAVE_RINTL = 0 || test $REPLACE_RINTL = 1; then
    dnl Find libraries needed to link lib/rintl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_RINT])
      RINTL_LIBM="$RINT_LIBM"
    else
      RINTL_LIBM=
    fi
  fi
  AC_SUBST([RINTL_LIBM])
])
