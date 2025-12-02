# hypotl.m4 serial 11
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_HYPOTL],
[
  m4_divert_text([DEFAULTS], [gl_hypotl_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_HYPOT])

  dnl Persuade glibc <math.h> to declare hypotl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether hypotl() exists. Assume that hypotl(), if it exists, is
  dnl defined in the same library as hypot().
  save_LIBS="$LIBS"
  LIBS="$LIBS $HYPOT_LIBM"
  AC_CHECK_FUNCS([hypotl])
  LIBS="$save_LIBS"
  if test $ac_cv_func_hypotl = yes; then
    HYPOTL_LIBM="$HYPOT_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $HYPOTL_LIBM"
    gl_FUNC_HYPOTL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_hypotl_works" in
      *yes) ;;
      *) REPLACE_HYPOTL=1 ;;
    esac

    m4_ifdef([gl_FUNC_HYPOTL_IEEE], [
      if test $gl_hypotl_required = ieee && test $REPLACE_HYPOTL = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether hypotl works according to ISO C 99 with IEC 60559],
          [gl_cv_func_hypotl_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $HYPOTL_LIBM"
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
numeric_equal (long double x, long double y)
{
  return x == y;
}
static long double dummy (long double x, long double y) { return 0; }
long double zero;
long double one = 1.0L;
int main (int argc, char *argv[])
{
  long double (* volatile my_hypotl) (long double, long double) = argc ? hypotl : dummy;
  long double f;
  /* Test hypotl(NaN,Infinity).
     This test fails on OSF/1 5.1 and native Windows.  */
  f = my_hypotl (zero / zero, one / zero);
  if (!numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_hypotl_ieee=yes],
              [gl_cv_func_hypotl_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_hypotl_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_hypotl_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_hypotl_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_hypotl_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_hypotl_ieee" in
          *yes) ;;
          *) REPLACE_HYPOTL=1 ;;
        esac
      fi
    ])
  else
    HAVE_HYPOTL=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_hypotl, not hypotl.
    AC_CHECK_DECLS([hypotl], [REPLACE_HYPOTL=1], , [[#include <math.h>]])
  fi
  if test $HAVE_HYPOTL = 0 || test $REPLACE_HYPOTL = 1; then
    dnl Find libraries needed to link lib/hypotl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      HYPOTL_LIBM="$HYPOT_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FABSL])
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      AC_REQUIRE([gl_FUNC_SQRTL])
      HYPOTL_LIBM=
      dnl Append $FABSL_LIBM to HYPOTL_LIBM, avoiding gratuitous duplicates.
      case " $HYPOTL_LIBM " in
        *" $FABSL_LIBM "*) ;;
        *) HYPOTL_LIBM="$HYPOTL_LIBM $FABSL_LIBM" ;;
      esac
      dnl Append $FREXPL_LIBM to HYPOTL_LIBM, avoiding gratuitous duplicates.
      case " $HYPOTL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) HYPOTL_LIBM="$HYPOTL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to HYPOTL_LIBM, avoiding gratuitous duplicates.
      case " $HYPOTL_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) HYPOTL_LIBM="$HYPOTL_LIBM $LDEXPL_LIBM" ;;
      esac
      dnl Append $SQRTL_LIBM to HYPOTL_LIBM, avoiding gratuitous duplicates.
      case " $HYPOTL_LIBM " in
        *" $SQRTL_LIBM "*) ;;
        *) HYPOTL_LIBM="$HYPOTL_LIBM $SQRTL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([HYPOTL_LIBM])
])

dnl Test whether hypotl() works.
dnl On OpenBSD 5.1/SPARC,
dnl hypotl (2.5541394760659556563446062497337725156L, 7.7893454113437840832487794525518765265L)
dnl has rounding errors that eat up the last 8 to 9 decimal digits.
AC_DEFUN([gl_FUNC_HYPOTL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether hypotl works], [gl_cv_func_hypotl_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
static long double
my_ldexpl (long double x, int d)
{
  for (; d > 0; d--)
    x *= 2.0L;
  for (; d < 0; d++)
    x *= 0.5L;
  return x;
}
volatile long double x;
volatile long double y;
volatile long double z;
int main ()
{
  long double err;

  x = 2.5541394760659556563446062497337725156L;
  y = 7.7893454113437840832487794525518765265L;
  z = hypotl (x, y);
  err = z * z - (x * x + y * y);
  err = my_ldexpl (err, LDBL_MANT_DIG);
  if (err < 0)
    err = - err;
  if (err > 1000.0L)
    return 1;
  return 0;
}
]])],
        [gl_cv_func_hypotl_works=yes],
        [gl_cv_func_hypotl_works=no],
        [case "$host_os" in
           openbsd*) gl_cv_func_hypotl_works="guessing no" ;;
                     # Guess yes on native Windows.
           mingw*)   gl_cv_func_hypotl_works="guessing yes" ;;
           *)        gl_cv_func_hypotl_works="guessing yes" ;;
         esac
        ])
    ])
])
