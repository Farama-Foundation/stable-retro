# remainderf.m4 serial 13
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_REMAINDERF],
[
  m4_divert_text([DEFAULTS], [gl_remainderf_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_REMAINDER])

  dnl Persuade glibc <math.h> to declare remainderf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether remainderf() exists. Assume that remainderf(), if it exists, is
  dnl defined in the same library as remainder().
  save_LIBS="$LIBS"
  LIBS="$LIBS $REMAINDER_LIBM"
  AC_CACHE_CHECK([for remainderf],
    [gl_cv_func_remainderf],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             extern
             #ifdef __cplusplus
             "C"
             #endif
             float remainderf (float, float);
             float (*funcptr) (float, float) = remainderf;
             float x;
             float y;]],
           [[return funcptr (x, y) > 1
                    || remainderf (x, y) > 1;]])],
        [gl_cv_func_remainderf=yes],
        [gl_cv_func_remainderf=no])
    ])
  LIBS="$save_LIBS"
  if test $gl_cv_func_remainderf = yes; then
    REMAINDERF_LIBM="$REMAINDER_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $REMAINDERF_LIBM"
    gl_FUNC_REMAINDERF_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_remainderf_works" in
      *yes) ;;
      *) REPLACE_REMAINDERF=1 ;;
    esac

    m4_ifdef([gl_FUNC_REMAINDERF_IEEE], [
      if test $gl_remainderf_required = ieee && test $REPLACE_REMAINDERF = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether remainderf works according to ISO C 99 with IEC 60559],
          [gl_cv_func_remainderf_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $REMAINDERF_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
extern
#ifdef __cplusplus
"C"
#endif
float remainderf (float, float);
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
  float (* volatile my_remainderf) (float, float) = argc ? remainderf : dummy;
  float f;
  /* Test remainderf(...,0.0f).
     This test fails on OSF/1 5.1.  */
  f = my_remainderf (2.0f, 0.0f);
  if (numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_remainderf_ieee=yes],
              [gl_cv_func_remainderf_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_remainderf_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_remainderf_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_remainderf_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_remainderf_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_remainderf_ieee" in
          *yes) ;;
          *) REPLACE_REMAINDERF=1 ;;
        esac
      fi
    ])
  else
    HAVE_REMAINDERF=0
  fi
  if test $HAVE_REMAINDERF = 0 || test $REPLACE_REMAINDERF = 1; then
    dnl Find libraries needed to link lib/remainderf.c.
    if test $gl_cv_func_remainder_no_libm = yes \
       || test $gl_cv_func_remainder_in_libm = yes; then
      AC_DEFINE([HAVE_REMAINDER], [1],
        [Define to 1 if the remainder() function is available in libc or libm.])
      REMAINDERF_LIBM="$REMAINDER_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FABSF])
      AC_REQUIRE([gl_FUNC_FMODF])
      AC_REQUIRE([gl_FUNC_ISNANF])
      REMAINDERF_LIBM=
      dnl Append $FABSF_LIBM to REMAINDERF_LIBM, avoiding gratuitous duplicates.
      case " $REMAINDERF_LIBM " in
        *" $FABSF_LIBM "*) ;;
        *) REMAINDERF_LIBM="$REMAINDERF_LIBM $FABSF_LIBM" ;;
      esac
      dnl Append $FMODF_LIBM to REMAINDERF_LIBM, avoiding gratuitous duplicates.
      case " $REMAINDERF_LIBM " in
        *" $FMODF_LIBM "*) ;;
        *) REMAINDERF_LIBM="$REMAINDERF_LIBM $FMODF_LIBM" ;;
      esac
      dnl Append $ISNANF_LIBM to REMAINDERF_LIBM, avoiding gratuitous duplicates.
      case " $REMAINDERF_LIBM " in
        *" $ISNANF_LIBM "*) ;;
        *) REMAINDERF_LIBM="$REMAINDERF_LIBM $ISNANF_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([REMAINDERF_LIBM])
])

dnl Test whether remainderf() works.
dnl It runs into an endless loop on IRIX 6.5.
AC_DEFUN([gl_FUNC_REMAINDERF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CHECK_DECLS_ONCE([alarm])
  AC_CACHE_CHECK([whether remainderf works], [gl_cv_func_remainderf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
#if HAVE_DECL_ALARM
# include <signal.h>
# include <unistd.h>
#endif
extern
#ifdef __cplusplus
"C"
#endif
float remainderf (float, float);
volatile float x;
volatile float y;
float z;
int main ()
{
#if HAVE_DECL_ALARM
  signal (SIGALRM, SIG_DFL);
  alarm (5);
#endif
  /* This test fails on IRIX 6.5.  */
  x = 9.316161e+37f;
  y = 0.5475547314f;
  z = remainderf (x, y);
  return 0;
}
]])],
        [gl_cv_func_remainderf_works=yes],
        [gl_cv_func_remainderf_works=no],
        [case "$host_os" in
           irix*)  gl_cv_func_remainderf_works="guessing no" ;;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_remainderf_works="guessing yes" ;;
           *)      gl_cv_func_remainderf_works="guessing yes" ;;
         esac
        ])
    ])
])
