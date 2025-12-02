# fmod.m4 serial 9
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMOD],
[
  m4_divert_text([DEFAULTS], [gl_fmod_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Determine FMOD_LIBM.
  gl_COMMON_DOUBLE_MATHFUNC([fmod])

  m4_ifdef([gl_FUNC_FMOD_IEEE], [
    if test $gl_fmod_required = ieee && test $REPLACE_FMOD = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether fmod works according to ISO C 99 with IEC 60559],
        [gl_cv_func_fmod_ieee],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $FMOD_LIBM"
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
static double dummy (double x, double y) { return 0; }
int main (int argc, char *argv[])
{
  double (* volatile my_fmod) (double, double) = argc ? fmod : dummy;
  int result = 0;
  double f;
  /* Test fmod(...,0.0).
     This test fails on OSF/1 5.1.  */
  f = my_fmod (2.0, 0.0);
  if (numeric_equal (f, f))
    result |= 1;
  /* Test fmod(-0.0,...).
     This test fails on native Windows.  */
  f = my_fmod (minus_zerod, 2.0);
  if (!(f == 0.0) || (signbitd (minus_zerod) && !signbitd (f)))
    result |= 2;
  return result;
}
            ]])],
            [gl_cv_func_fmod_ieee=yes],
            [gl_cv_func_fmod_ieee=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_fmod_ieee="guessing yes" ;;
                              # Guess yes on musl systems.
               *-musl*)       gl_cv_func_fmod_ieee="guessing yes" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_fmod_ieee="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_fmod_ieee="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_fmod_ieee" in
        *yes) ;;
        *) REPLACE_FMOD=1 ;;
      esac
    fi
  ])
  if test $REPLACE_FMOD = 1; then
    dnl Find libraries needed to link lib/fmod.c.
    AC_REQUIRE([gl_FUNC_FABS])
    AC_REQUIRE([gl_FUNC_FREXP])
    AC_REQUIRE([gl_FUNC_TRUNC])
    AC_REQUIRE([gl_FUNC_LDEXP])
    AC_REQUIRE([gl_FUNC_ISNAND])
    FMOD_LIBM=
    dnl Append $FABS_LIBM to FMOD_LIBM, avoiding gratuitous duplicates.
    case " $FMOD_LIBM " in
      *" $FABS_LIBM "*) ;;
      *) FMOD_LIBM="$FMOD_LIBM $FABS_LIBM" ;;
    esac
    dnl Append $FREXP_LIBM to FMOD_LIBM, avoiding gratuitous duplicates.
    case " $FMOD_LIBM " in
      *" $FREXP_LIBM "*) ;;
      *) FMOD_LIBM="$FMOD_LIBM $FREXP_LIBM" ;;
    esac
    dnl Append $TRUNC_LIBM to FMOD_LIBM, avoiding gratuitous duplicates.
    case " $FMOD_LIBM " in
      *" $TRUNC_LIBM "*) ;;
      *) FMOD_LIBM="$FMOD_LIBM $TRUNC_LIBM" ;;
    esac
    dnl Append $LDEXP_LIBM to FMOD_LIBM, avoiding gratuitous duplicates.
    case " $FMOD_LIBM " in
      *" $LDEXP_LIBM "*) ;;
      *) FMOD_LIBM="$FMOD_LIBM $LDEXP_LIBM" ;;
    esac
    dnl Append $ISNAND_LIBM to FMOD_LIBM, avoiding gratuitous duplicates.
    case " $FMOD_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) FMOD_LIBM="$FMOD_LIBM $ISNAND_LIBM" ;;
    esac
  fi
])
