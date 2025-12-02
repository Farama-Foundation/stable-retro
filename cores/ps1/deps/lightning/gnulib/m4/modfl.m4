# modfl.m4 serial 8
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MODFL],
[
  m4_divert_text([DEFAULTS], [gl_modfl_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare modfl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether modfl() exists. We cannot assume that modfl(), if it
  dnl exists, is defined in the same library as modf(). This is not the case
  dnl on FreeBSD, Solaris.
  gl_MATHFUNC([modfl], [long double], [(long double, long double *)])
  if test $gl_cv_func_modfl_no_libm = yes \
     || test $gl_cv_func_modfl_in_libm = yes; then
    :
    m4_ifdef([gl_FUNC_MODFL_IEEE], [
      if test $gl_modfl_required = ieee && test $REPLACE_MODFL = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether modfl works according to ISO C 99 with IEC 60559],
          [gl_cv_func_modfl_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $MODFL_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_LONG_DOUBLE_MINUS_ZERO_CODE[
]gl_LONG_DOUBLE_SIGNBIT_CODE[
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (long double x, long double y)
{
  return x == y;
}
static long double dummy (long double x, long double *iptr) { return 0; }
long double zero;
long double minus_one = - 1.0L;
int main (int argc, char *argv[])
{
  long double (* volatile my_modfl) (long double, long double *) = argc ? modfl : dummy;
  long double i;
  long double f;
  /* Test modfl(-Inf,...).
     This test fails on IRIX 6.5, OSF/1 5.1, mingw.  */
  f = my_modfl (minus_one / zero, &i);
  if (!(f == 0.0L) || (signbitl (minus_zerol) && !signbitl (f)))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_modfl_ieee=yes],
              [gl_cv_func_modfl_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_modfl_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_modfl_ieee="guessing yes" ;;
                                # Guess yes on MSVC, no on mingw.
                 mingw*)        AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
                                  ],
                                  [gl_cv_func_modfl_ieee="guessing yes"],
                                  [gl_cv_func_modfl_ieee="guessing no"])
                                ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_modfl_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_modfl_ieee" in
          *yes) ;;
          *) REPLACE_MODFL=1 ;;
        esac
      fi
    ])
  else
    HAVE_MODFL=0
  fi
  if test $HAVE_MODFL = 0 || test $REPLACE_MODFL = 1; then
    dnl Find libraries needed to link lib/modfl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_MODF])
      MODFL_LIBM="$MODF_LIBM"
    else
      AC_REQUIRE([gl_FUNC_TRUNCL])
      MODFL_LIBM="$TRUNCL_LIBM"
    fi
  fi
  AC_SUBST([MODFL_LIBM])
])
