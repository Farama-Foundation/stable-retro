# fmodf.m4 serial 11
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMODF],
[
  m4_divert_text([DEFAULTS], [gl_fmodf_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_FMOD])

  dnl Persuade glibc <math.h> to declare fmodf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether fmodf() exists. Assume that fmodf(), if it exists, is
  dnl defined in the same library as fmod().
  save_LIBS="$LIBS"
  LIBS="$LIBS $FMOD_LIBM"
  AC_CHECK_FUNCS([fmodf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_fmodf = yes; then
    FMODF_LIBM="$FMOD_LIBM"
    m4_ifdef([gl_FUNC_FMODF_IEEE], [
      if test $gl_fmodf_required = ieee && test $REPLACE_FMODF = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether fmodf works according to ISO C 99 with IEC 60559],
          [gl_cv_func_fmodf_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $FMODF_LIBM"
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
int main (int argc, char *argv[])
{
  float (* volatile my_fmodf) (float, float) = argc ? fmodf : dummy;
  float f;
  /* Test fmodf(...,0.0f).
     This test fails on OSF/1 5.1.  */
  f = my_fmodf (2.0f, 0.0f);
  if (numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_fmodf_ieee=yes],
              [gl_cv_func_fmodf_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_fmodf_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_fmodf_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_fmodf_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_fmodf_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_fmodf_ieee" in
          *yes) ;;
          *) REPLACE_FMODF=1 ;;
        esac
      fi
    ])
  else
    HAVE_FMODF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_fmodf, not fmodf.
    AC_CHECK_DECLS([fmodf], [REPLACE_FMODF=1], , [[#include <math.h>]])
  fi
  if test $HAVE_FMODF = 0 || test $REPLACE_FMODF = 1; then
    dnl Find libraries needed to link lib/fmodf.c.
    FMODF_LIBM="$FMOD_LIBM"
  fi
  AC_SUBST([FMODF_LIBM])
])
