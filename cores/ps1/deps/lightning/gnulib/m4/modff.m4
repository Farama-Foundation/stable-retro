# modff.m4 serial 11
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MODFF],
[
  m4_divert_text([DEFAULTS], [gl_modff_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_MODF])

  dnl Persuade glibc <math.h> to declare modff().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether modff() exists. We cannot assume that modff(), if it
  dnl exists, is defined in the same library as modf(). This is not the case
  dnl on FreeBSD, NetBSD, OpenBSD.
  gl_MATHFUNC([modff], [float], [(float, float *)])
  if test $gl_cv_func_modff_no_libm = yes \
     || test $gl_cv_func_modff_in_libm = yes; then
    :
    m4_ifdef([gl_FUNC_MODFF_IEEE], [
      if test $gl_modff_required = ieee && test $REPLACE_MODFF = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether modff works according to ISO C 99 with IEC 60559],
          [gl_cv_func_modff_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $MODFF_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_FLOAT_MINUS_ZERO_CODE[
]gl_FLOAT_SIGNBIT_CODE[
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (float x, float y)
{
  return x == y;
}
static float dummy (float x, float *iptr) { return 0; }
float zero;
float minus_one = - 1.0f;
int main (int argc, char *argv[])
{
  float (* volatile my_modff) (float, float *) = argc ? modff : dummy;
  int result = 0;
  float i;
  float f;
  /* Test modff(NaN,...).
     This test fails on NetBSD 5.1, OpenBSD 6.7, Solaris 9, Cygwin.  */
  f = my_modff (zero / zero, &i);
  if (numeric_equal (f, f))
    result |= 1;
  /* Test modff(-Inf,...).
     This test fails on IRIX 6.5, OSF/1 5.1, mingw.  */
  f = my_modff (minus_one / zero, &i);
  if (!(f == 0.0f) || (signbitf (minus_zerof) && !signbitf (f)))
    result |= 2;
  return result;
}
              ]])],
              [gl_cv_func_modff_ieee=yes],
              [gl_cv_func_modff_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_modff_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_modff_ieee="guessing yes" ;;
                                # Guess yes on MSVC, no on mingw.
                 mingw*)        AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
                                  ],
                                  [gl_cv_func_modff_ieee="guessing yes"],
                                  [gl_cv_func_modff_ieee="guessing no"])
                                ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_modff_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_modff_ieee" in
          *yes) ;;
          *) REPLACE_MODFF=1 ;;
        esac
      fi
    ])
  else
    HAVE_MODFF=0
  fi
  if test $HAVE_MODFF = 0 || test $REPLACE_MODFF = 1; then
    dnl Find libraries needed to link lib/modff.c.
    MODFF_LIBM="$MODF_LIBM"
  fi
  AC_SUBST([MODFF_LIBM])
])
