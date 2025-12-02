# logb.m4 serial 9
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOGB],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare logb().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether logb() is declared.
  AC_CHECK_DECLS([logb], , , [[#include <math.h>]])
  if test "$ac_cv_have_decl_logb" != yes; then
    HAVE_DECL_LOGB=0
  fi
  dnl Test whether logb() can be used without libm.
  LOGB_LIBM=?
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
         double logb (double x);
         double x;]],
       [[x = logb(x);]])],
    [LOGB_LIBM=])
  if test "$LOGB_LIBM" = "?"; then
    save_LIBS="$LIBS"
    LIBS="$LIBS -lm"
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
           double logb (double x);
           double x;]],
         [[x = logb(x);]])],
      [LOGB_LIBM="-lm"])
    LIBS="$save_LIBS"
  fi
  if test "$LOGB_LIBM" != "?"; then
    HAVE_LOGB=1
    save_LIBS="$LIBS"
    LIBS="$LIBS $LOGB_LIBM"
    gl_FUNC_LOGB_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_logb_works" in
      *yes) ;;
      *) REPLACE_LOGB=1 ;;
    esac
  else
    HAVE_LOGB=0
  fi
  if test $HAVE_LOGB = 0 || test $REPLACE_LOGB = 1; then
    dnl Find libraries needed to link lib/logb.c.
    AC_REQUIRE([gl_FUNC_FREXP])
    AC_REQUIRE([gl_FUNC_ISNAND])
    LOGB_LIBM=
    dnl Append $FREXP_LIBM to LOGB_LIBM, avoiding gratuitous duplicates.
    case " $LOGB_LIBM " in
      *" $FREXP_LIBM "*) ;;
      *) LOGB_LIBM="$LOGB_LIBM $FREXP_LIBM" ;;
    esac
    dnl Append $ISNAND_LIBM to LOGB_LIBM, avoiding gratuitous duplicates.
    case " $LOGB_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) LOGB_LIBM="$LOGB_LIBM $ISNAND_LIBM" ;;
    esac
  fi
  AC_SUBST([LOGB_LIBM])
])

dnl Test whether logb() works.
dnl On glibc 2.11/ppc, glibc 2.7/sparc, glibc 2.7/hppa, Solaris 10/SPARC,
dnl Solaris 11.4/x86_64, Cygwin 1.5.x, the return value for subnormal
dnl (denormalized) arguments is too large.
dnl On glibc 2.17/ppc likewise but only for negative subnormal (denormalized)
dnl arguments.
AC_DEFUN([gl_FUNC_LOGB_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether logb works], [gl_cv_func_logb_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
extern
#ifdef __cplusplus
"C"
#endif
double logb (double);
volatile double x;
int main ()
{
  int result = 0;
  /* This test fails on glibc 2.11/ppc, glibc 2.7/sparc, glibc 2.7/hppa,
     Solaris 10/SPARC, Solaris 11.4/x86_64, Cygwin 1.5.x.  */
  {
    int i;
    for (i = 1, x = 1.0; i >= DBL_MIN_EXP; i--, x *= 0.5)
      ;
    /* Here i = DBL_MIN_EXP - 1. Either x = 2^(i-1) is subnormal or x = 0.0.  */
    if (x > 0.0 && !(logb (x) == (double)(i - 1)))
      result |= 1;
  }
  /* This test fails on glibc 2.17/ppc.  */
  {
    int i;
    for (i = 1, x = -1.0; i >= DBL_MIN_EXP; i--, x *= 0.5)
      ;
    /* Here i = DBL_MIN_EXP - 1. Either x = -2^(i-1) is subnormal or x = -0.0.  */
    if (x < 0.0 && !(logb (x) == (double)(i - 1)))
      result |= 2;
  }
  return result;
}
]])],
        [gl_cv_func_logb_works=yes],
        [gl_cv_func_logb_works=no],
        [case "$host_os" in
           *gnu* | solaris* | cygwin*) gl_cv_func_logb_works="guessing no" ;;
           mingw*) # Guess yes on MSVC, no on mingw.
             AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
               ],
               [gl_cv_func_logb_works="guessing yes"],
               [gl_cv_func_logb_works="guessing no"])
             ;;
           *) gl_cv_func_logb_works="guessing yes" ;;
         esac
        ])
    ])
])
