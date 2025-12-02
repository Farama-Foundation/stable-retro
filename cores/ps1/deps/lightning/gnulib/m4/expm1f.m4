# expm1f.m4 serial 4
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXPM1F],
[
  m4_divert_text([DEFAULTS], [gl_expm1f_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_EXPM1])

  dnl Persuade glibc <math.h> to declare expm1f().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether expm1f() exists. Assume that expm1f(), if it exists, is
  dnl defined in the same library as expm1().
  save_LIBS="$LIBS"
  LIBS="$LIBS $EXPM1_LIBM"
  AC_CHECK_FUNCS([expm1f])
  LIBS="$save_LIBS"
  if test $ac_cv_func_expm1f = yes; then
    EXPM1F_LIBM="$EXPM1_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $EXPM1F_LIBM"
    gl_FUNC_EXPM1F_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_expm1f_works" in
      *yes) ;;
      *) REPLACE_EXPM1F=1 ;;
    esac

    m4_ifdef([gl_FUNC_EXPM1F_IEEE], [
      if test $gl_expm1f_required = ieee && test $REPLACE_EXPM1F = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether expm1f works according to ISO C 99 with IEC 60559],
          [gl_cv_func_expm1f_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $EXPM1F_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_FLOAT_MINUS_ZERO_CODE[
]gl_FLOAT_SIGNBIT_CODE[
#ifndef expm1f
extern
#ifdef __cplusplus
"C"
#endif
float expm1f (float);
#endif
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (float x, float y)
{
  return x == y;
}
static float dummy (float x) { return 0; }
int main (int argc, char *argv[])
{
  float (* volatile my_expm1f) (float) = argc ? expm1f : dummy;
  /* Test expm1f(-0.0f).
     This test fails on AIX 7.2.  */
  float y = my_expm1f (minus_zerof);
  if (signbitf (minus_zerof) && !signbitf (y))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_expm1f_ieee=yes],
              [gl_cv_func_expm1f_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_expm1f_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_expm1f_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_expm1f_ieee="guessing yes" ;;
                                # Guess no on AIX.
                 aix*)          gl_cv_func_expm1f_ieee="guessing no" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_expm1f_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_expm1f_ieee" in
          *yes) ;;
          *) REPLACE_EXPM1F=1 ;;
        esac
      fi
    ])
  else
    HAVE_EXPM1F=0
  fi
  if test $HAVE_EXPM1F = 0 || test $REPLACE_EXPM1F = 1; then
    dnl Find libraries needed to link lib/expm1f.c.
    EXPM1F_LIBM="$EXPM1_LIBM"
  fi
  AC_SUBST([EXPM1F_LIBM])
])

dnl Test whether expm1f() works.
dnl On IRIX 6.5, for arguments <= -17.32868, it returns -5.6295e14.
AC_DEFUN([gl_FUNC_EXPM1F_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether expm1f works], [gl_cv_func_expm1f_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
volatile float x;
float y;
int main ()
{
  x = -100.0f;
  y = expm1f (x);
  if (y < -1.0f)
    return 1;
  return 0;
}
]])],
        [gl_cv_func_expm1f_works=yes],
        [gl_cv_func_expm1f_works=no],
        [case "$host_os" in
           irix*)  gl_cv_func_expm1f_works="guessing no" ;;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_expm1f_works="guessing yes" ;;
           *)      gl_cv_func_expm1f_works="guessing yes" ;;
         esac
        ])
    ])
])
