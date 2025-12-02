# log10f.m4 serial 12
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOG10F],
[
  m4_divert_text([DEFAULTS], [gl_log10f_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_LOG10])

  dnl Persuade glibc <math.h> to declare log10f().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether log10f() exists. Assume that log10f(), if it exists, is
  dnl defined in the same library as log10().
  save_LIBS="$LIBS"
  LIBS="$LIBS $LOG10_LIBM"
  AC_CHECK_FUNCS([log10f])
  LIBS="$save_LIBS"
  if test $ac_cv_func_log10f = yes; then
    LOG10F_LIBM="$LOG10_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $LOG10F_LIBM"
    gl_FUNC_LOG10F_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_log10f_works" in
      *yes) ;;
      *) REPLACE_LOG10F=1 ;;
    esac

    m4_ifdef([gl_FUNC_LOG10F_IEEE], [
      if test $gl_log10f_required = ieee && test $REPLACE_LOG10F = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether log10f works according to ISO C 99 with IEC 60559],
          [gl_cv_func_log10f_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $LOG10F_LIBM"
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
numeric_equal (float x, float y)
{
  return x == y;
}
static float dummy (float x) { return 0; }
int main (int argc, char *argv[])
{
  float (* volatile my_log10f) (float) = argc ? log10f : dummy;
  /* Test log10f(negative).
     This test fails on NetBSD 5.1.  */
  float y = my_log10f (-1.0f);
  if (numeric_equal (y, y))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_log10f_ieee=yes],
              [gl_cv_func_log10f_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_log10f_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_log10f_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_log10f_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_log10f_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_log10f_ieee" in
          *yes) ;;
          *) REPLACE_LOG10F=1 ;;
        esac
      fi
    ])
  else
    HAVE_LOG10F=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_log10f, not log10f.
    AC_CHECK_DECLS([log10f], [REPLACE_LOG10F=1], , [[#include <math.h>]])
  fi
  if test $HAVE_LOG10F = 0 || test $REPLACE_LOG10F = 1; then
    dnl Find libraries needed to link lib/log10f.c.
    if test $HAVE_LOG10F = 0; then
      LOG10F_LIBM="$LOG10_LIBM"
    fi
  fi
  AC_SUBST([LOG10F_LIBM])
])

dnl Test whether log10f() works.
dnl On OSF/1 5.1, log10f(-0.0f) is NaN.
AC_DEFUN([gl_FUNC_LOG10F_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether log10f works], [gl_cv_func_log10f_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
volatile float x;
float y;
int main ()
{
  x = -0.0f;
  y = log10f (x);
  if (!(y + y == y))
    return 1;
  return 0;
}
]])],
        [gl_cv_func_log10f_works=yes],
        [gl_cv_func_log10f_works=no],
        [case "$host_os" in
           osf*)   gl_cv_func_log10f_works="guessing no" ;;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_log10f_works="guessing yes" ;;
           *)      gl_cv_func_log10f_works="guessing yes" ;;
         esac
        ])
    ])
])
