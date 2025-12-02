# remainder.m4 serial 9
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_REMAINDER],
[
  m4_divert_text([DEFAULTS], [gl_remainder_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Test whether remainder() is declared. On IRIX 5.3 it is not declared.
  AC_CHECK_DECL([remainder], , [HAVE_DECL_REMAINDER=0], [[#include <math.h>]])

  REMAINDER_LIBM=
  AC_CACHE_CHECK([whether remainder() can be used without linking with libm],
    [gl_cv_func_remainder_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             double x;
             double y;]],
           [[return remainder (x, y) > 1;]])],
        [gl_cv_func_remainder_no_libm=yes],
        [gl_cv_func_remainder_no_libm=no])
    ])
  if test $gl_cv_func_remainder_no_libm = no; then
    AC_CACHE_CHECK([whether remainder() can be used with libm],
      [gl_cv_func_remainder_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               double x;
               double y;]],
             [[return remainder (x, y) > 1;]])],
          [gl_cv_func_remainder_in_libm=yes],
          [gl_cv_func_remainder_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_remainder_in_libm = yes; then
      REMAINDER_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_remainder_no_libm = yes \
     || test $gl_cv_func_remainder_in_libm = yes; then
    :
    m4_ifdef([gl_FUNC_REMAINDER_IEEE], [
      if test $gl_remainder_required = ieee && test $REPLACE_REMAINDER = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether remainder works according to ISO C 99 with IEC 60559],
          [gl_cv_func_remainder_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $REMAINDER_LIBM"
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
int main (int argc, char *argv[])
{
  double (* volatile my_remainder) (double, double) = argc ? remainder : dummy;
  double f;
  /* Test remainder(...,0.0).
     This test fails on OSF/1 5.1.  */
  f = my_remainder (2.0, 0.0);
  if (numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_remainder_ieee=yes],
              [gl_cv_func_remainder_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_remainder_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_remainder_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_remainder_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_remainder_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_remainder_ieee" in
          *yes) ;;
          *) REPLACE_REMAINDER=1 ;;
        esac
      fi
    ])
  else
    HAVE_REMAINDER=0
  fi
  if test $HAVE_REMAINDER = 0 || test $REPLACE_REMAINDER = 1; then
    dnl Find libraries needed to link lib/remainder.c.
    AC_REQUIRE([gl_FUNC_FABS])
    AC_REQUIRE([gl_FUNC_FMOD])
    AC_REQUIRE([gl_FUNC_ISNAND])
    REMAINDER_LIBM=
    dnl Append $FABS_LIBM to REMAINDER_LIBM, avoiding gratuitous duplicates.
    case " $REMAINDER_LIBM " in
      *" $FABS_LIBM "*) ;;
      *) REMAINDER_LIBM="$REMAINDER_LIBM $FABS_LIBM" ;;
    esac
    dnl Append $FMOD_LIBM to REMAINDER_LIBM, avoiding gratuitous duplicates.
    case " $REMAINDER_LIBM " in
      *" $FMOD_LIBM "*) ;;
      *) REMAINDER_LIBM="$REMAINDER_LIBM $FMOD_LIBM" ;;
    esac
    dnl Append $ISNAND_LIBM to REMAINDER_LIBM, avoiding gratuitous duplicates.
    case " $REMAINDER_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) REMAINDER_LIBM="$REMAINDER_LIBM $ISNAND_LIBM" ;;
    esac
  fi
  AC_SUBST([REMAINDER_LIBM])
])
