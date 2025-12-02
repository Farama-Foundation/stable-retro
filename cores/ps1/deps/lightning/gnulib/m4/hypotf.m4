# hypotf.m4 serial 10
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_HYPOTF],
[
  m4_divert_text([DEFAULTS], [gl_hypotf_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_HYPOT])

  dnl Persuade glibc <math.h> to declare hypotf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether hypotf() exists. Assume that hypotf(), if it exists, is
  dnl defined in the same library as hypot().
  save_LIBS="$LIBS"
  LIBS="$LIBS $HYPOT_LIBM"
  AC_CHECK_FUNCS([hypotf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_hypotf = yes; then
    HYPOTF_LIBM="$HYPOT_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $HYPOTF_LIBM"
    gl_FUNC_HYPOTF_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_hypotf_works" in
      *yes) ;;
      *) REPLACE_HYPOTF=1 ;;
    esac

    m4_ifdef([gl_FUNC_HYPOTF_IEEE], [
      if test $gl_hypotf_required = ieee && test $REPLACE_HYPOTF = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether hypotf works according to ISO C 99 with IEC 60559],
          [gl_cv_func_hypotf_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $HYPOTF_LIBM"
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
static float dummy (float x, float y) { return 0; }
float zero;
float one = 1.0f;
int main (int argc, char *argv[])
{
  float (* volatile my_hypotf) (float, float) = argc ? hypotf : dummy;
  float f;
  /* Test hypotf(NaN,Infinity).
     This test fails on OSF/1 5.1 and native Windows.  */
  f = my_hypotf (zero / zero, one / zero);
  if (!numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_hypotf_ieee=yes],
              [gl_cv_func_hypotf_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_hypotf_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_hypotf_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_hypotf_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_hypotf_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_hypotf_ieee" in
          *yes) ;;
          *) REPLACE_HYPOTF=1 ;;
        esac
      fi
    ])
  else
    HAVE_HYPOTF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_hypotf, not hypotf.
    AC_CHECK_DECLS([hypotf], [REPLACE_HYPOTF=1], , [[#include <math.h>]])
  fi
  if test $HAVE_HYPOTF = 0 || test $REPLACE_HYPOTF = 1; then
    dnl Find libraries needed to link lib/hypotf.c.
    HYPOTF_LIBM="$HYPOT_LIBM"
  fi
  AC_SUBST([HYPOTF_LIBM])
])

dnl Test whether hypotf() works.
dnl It returns wrong values on NetBSD 5.1/x86_64 and OpenBSD 4.9/x86.
AC_DEFUN([gl_FUNC_HYPOTF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether hypotf works], [gl_cv_func_hypotf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
volatile float x;
volatile float y;
volatile float z;
int main ()
{
  int result = 0;
  /* This test fails on NetBSD 5.1.  */
  {
    x = FLT_MIN * 2.0f;
    y = FLT_MIN * 3.0f;
    z = hypotf (x, y);
    if (!(z >= FLT_MIN * 2.0f && z <= FLT_MIN * 4.0f))
      result |= 1;
  }
  /* This test fails on OpenBSD 4.9.  */
  {
    x = FLT_MAX;
    y = FLT_MAX * 0.5f;
    z = hypotf (x, y);
    if (!(z > 0 && z == z + z))
      result |= 2;
  }
  return result;
}
]])],
        [gl_cv_func_hypotf_works=yes],
        [gl_cv_func_hypotf_works=no],
        [case "$host_os" in
           netbsd* | openbsd*) gl_cv_func_hypotf_works="guessing no" ;;
                               # Guess yes on native Windows.
           mingw*)             gl_cv_func_hypotf_works="guessing yes" ;;
           *)                  gl_cv_func_hypotf_works="guessing yes" ;;
         esac
        ])
    ])
])
