# log1p.m4 serial 8
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOG1P],
[
  m4_divert_text([DEFAULTS], [gl_log1p_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare log1p().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine LOG1P_LIBM.
  gl_COMMON_DOUBLE_MATHFUNC([log1p])

  dnl Test whether log1p() exists.
  save_LIBS="$LIBS"
  LIBS="$LIBS $LOG1P_LIBM"
  AC_CHECK_FUNCS([log1p])
  LIBS="$save_LIBS"
  if test $ac_cv_func_log1p = yes; then
    :
    m4_ifdef([gl_FUNC_LOG1P_IEEE], [
      if test $gl_log1p_required = ieee && test $REPLACE_LOG1P = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether log1p works according to ISO C 99 with IEC 60559],
          [gl_cv_func_log1p_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $LOG1P_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_DOUBLE_MINUS_ZERO_CODE[
]gl_DOUBLE_SIGNBIT_CODE[
static double dummy (double x) { return 0; }
int main (int argc, char *argv[])
{
  double (* volatile my_log1p) (double) = argc ? log1p : dummy;
  /* This test fails on AIX, HP-UX 11.  */
  double y = my_log1p (minus_zerod);
  if (!(y == 0.0) || (signbitd (minus_zerod) && !signbitd (y)))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_log1p_ieee=yes],
              [gl_cv_func_log1p_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_log1p_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_log1p_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_log1p_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_log1p_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_log1p_ieee" in
          *yes) ;;
          *) REPLACE_LOG1P=1 ;;
        esac
      fi
    ])
  else
    HAVE_LOG1P=0
  fi
  if test $HAVE_LOG1P = 0 || test $REPLACE_LOG1P = 1; then
    dnl Find libraries needed to link lib/log1p.c.
    AC_REQUIRE([gl_FUNC_ISNAND])
    AC_REQUIRE([gl_FUNC_LOG])
    AC_REQUIRE([gl_FUNC_ROUND])
    LOG1P_LIBM=
    dnl Append $ISNAND_LIBM to LOG1P_LIBM, avoiding gratuitous duplicates.
    case " $LOG1P_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) LOG1P_LIBM="$LOG1P_LIBM $ISNAND_LIBM" ;;
    esac
    dnl Append $LOG_LIBM to LOG1P_LIBM, avoiding gratuitous duplicates.
    case " $LOG1P_LIBM " in
      *" $LOG_LIBM "*) ;;
      *) LOG1P_LIBM="$LOG1P_LIBM $LOG_LIBM" ;;
    esac
    dnl Append $ROUND_LIBM to LOG1P_LIBM, avoiding gratuitous duplicates.
    case " $LOG1P_LIBM " in
      *" $ROUND_LIBM "*) ;;
      *) LOG1P_LIBM="$LOG1P_LIBM $ROUND_LIBM" ;;
    esac
  fi
])
