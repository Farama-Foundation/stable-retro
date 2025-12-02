# hypot.m4 serial 9
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_HYPOT],
[
  m4_divert_text([DEFAULTS], [gl_hypot_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare hypot().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine HYPOT_LIBM.
  gl_COMMON_DOUBLE_MATHFUNC([hypot])

  m4_ifdef([gl_FUNC_HYPOT_IEEE], [
    if test $gl_hypot_required = ieee && test $REPLACE_HYPOT = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether hypot works according to ISO C 99 with IEC 60559],
        [gl_cv_func_hypot_ieee],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $HYPOT_LIBM"
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (double x, double y)
{
  return x == y;
}
static double dummy (double x, double y) { return 0; }
double zero;
double one = 1.0;
int main (int argc, char *argv[])
{
  double (* volatile my_hypot) (double, double) = argc ? hypot : dummy;
  double f;
  /* Test hypot(NaN,Infinity).
     This test fails on OSF/1 5.1 and native Windows.  */
  f = my_hypot (zero / zero, one / zero);
  if (!numeric_equal (f, f))
    return 1;
  return 0;
}
            ]])],
            [gl_cv_func_hypot_ieee=yes],
            [gl_cv_func_hypot_ieee=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_hypot_ieee="guessing yes" ;;
                              # Guess yes on musl systems.
               *-musl*)       gl_cv_func_hypot_ieee="guessing yes" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_hypot_ieee="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_hypot_ieee="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_hypot_ieee" in
        *yes) ;;
        *) REPLACE_HYPOT=1 ;;
      esac
    fi
  ])
  if test $REPLACE_HYPOT = 1; then
    dnl Find libraries needed to link lib/hypot.c.
    AC_REQUIRE([gl_FUNC_FABS])
    AC_REQUIRE([gl_FUNC_FREXP])
    AC_REQUIRE([gl_FUNC_LDEXP])
    AC_REQUIRE([gl_FUNC_SQRT])
    HYPOT_LIBM=
    dnl Append $FABS_LIBM to HYPOT_LIBM, avoiding gratuitous duplicates.
    case " $HYPOT_LIBM " in
      *" $FABS_LIBM "*) ;;
      *) HYPOT_LIBM="$HYPOT_LIBM $FABS_LIBM" ;;
    esac
    dnl Append $FREXP_LIBM to HYPOT_LIBM, avoiding gratuitous duplicates.
    case " $HYPOT_LIBM " in
      *" $FREXP_LIBM "*) ;;
      *) HYPOT_LIBM="$HYPOT_LIBM $FREXP_LIBM" ;;
    esac
    dnl Append $LDEXP_LIBM to HYPOT_LIBM, avoiding gratuitous duplicates.
    case " $HYPOT_LIBM " in
      *" $LDEXP_LIBM "*) ;;
      *) HYPOT_LIBM="$HYPOT_LIBM $LDEXP_LIBM" ;;
    esac
    dnl Append $SQRT_LIBM to HYPOT_LIBM, avoiding gratuitous duplicates.
    case " $HYPOT_LIBM " in
      *" $SQRT_LIBM "*) ;;
      *) HYPOT_LIBM="$HYPOT_LIBM $SQRT_LIBM" ;;
    esac
  fi
])
