# modf.m4 serial 9
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MODF],
[
  m4_divert_text([DEFAULTS], [gl_modf_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Determine MODF_LIBM.
  gl_MATHFUNC([modf], [double], [(double, double *)])

  m4_ifdef([gl_FUNC_MODF_IEEE], [
    if test $gl_modf_required = ieee && test $REPLACE_MODF = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether modf works according to ISO C 99 with IEC 60559],
        [gl_cv_func_modf_ieee],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $MODF_LIBM"
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_DOUBLE_MINUS_ZERO_CODE[
]gl_DOUBLE_SIGNBIT_CODE[
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (double x, double y)
{
  return x == y;
}
static double dummy (double x, double *iptr) { return 0; }
double zero;
double minus_one = - 1.0;
int main (int argc, char *argv[])
{
  double (* volatile my_modf) (double, double *) = argc ? modf : dummy;
  int result = 0;
  double i;
  double f;
  /* Test modf(NaN,...).
     This test fails on NetBSD 5.1, Cygwin.  */
  f = my_modf (zero / zero, &i);
  if (numeric_equal (f, f))
    result |= 1;
  /* Test modf(-Inf,...).
     This test fails on FreeBSD 6.4, OpenBSD 6.7, IRIX 6.5, OSF/1 5.1.  */
  f = my_modf (minus_one / zero, &i);
  if (!(f == 0.0) || (signbitd (minus_zerod) && !signbitd (f)))
    result |= 2;
  return result;
}
            ]])],
            [gl_cv_func_modf_ieee=yes],
            [gl_cv_func_modf_ieee=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_modf_ieee="guessing yes" ;;
                              # Guess yes on musl systems.
               *-musl*)       gl_cv_func_modf_ieee="guessing yes" ;;
                              # Guess yes on MSVC, no on mingw.
               mingw*)        AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
                                ],
                                [gl_cv_func_modf_ieee="guessing yes"],
                                [gl_cv_func_modf_ieee="guessing no"])
                              ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_modf_ieee="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_modf_ieee" in
        *yes) ;;
        *) REPLACE_MODF=1 ;;
      esac
    fi
  ])
  if test $REPLACE_MODF = 1; then
    dnl Find libraries needed to link lib/modf.c.
    AC_REQUIRE([gl_FUNC_TRUNC])
    MODF_LIBM="$TRUNC_LIBM"
  fi
])
