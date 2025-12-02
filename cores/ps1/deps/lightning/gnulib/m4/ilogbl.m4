# ilogbl.m4 serial 5
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ILOGBL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_ILOGB])

  dnl Persuade glibc <math.h> to declare ilogbl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether ilogbl() exists. Assume that ilogbl(), if it exists, is
  dnl defined in the same library as ilogb().
  save_LIBS="$LIBS"
  LIBS="$LIBS $ILOGB_LIBM"
  AC_CACHE_CHECK([for ilogbl],
    [gl_cv_func_ilogbl],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             int (*funcptr) (long double) = ilogbl;
             long double x;]],
           [[return (funcptr (x) % 2) == 0
                    || (ilogbl (x) % 2) == 0;]])],
        [gl_cv_func_ilogbl=yes],
        [gl_cv_func_ilogbl=no])
    ])
  LIBS="$save_LIBS"
  if test $gl_cv_func_ilogbl = yes; then
    ILOGBL_LIBM="$ILOGB_LIBM"
    save_LIBS="$LIBS"
    LIBS="$LIBS $ILOGBL_LIBM"
    gl_FUNC_ILOGBL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_ilogbl_works" in
      *yes) ;;
      *) REPLACE_ILOGBL=1 ;;
    esac
  else
    HAVE_ILOGBL=0
  fi
  if test $HAVE_ILOGBL = 0 || test $REPLACE_ILOGBL = 1; then
    dnl Find libraries needed to link lib/ilogbl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      ILOGBL_LIBM="$ILOGB_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_ISNANL])
      ILOGBL_LIBM=
      dnl Append $FREXPL_LIBM to ILOGBL_LIBM, avoiding gratuitous duplicates.
      case " $ILOGBL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) ILOGBL_LIBM="$ILOGBL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $ISNANL_LIBM to ILOGBL_LIBM, avoiding gratuitous duplicates.
      case " $ILOGBL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) ILOGBL_LIBM="$ILOGBL_LIBM $ISNANL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([ILOGBL_LIBM])
])

dnl Test whether ilogbl() works.
dnl On Cygwin 2.9, ilogbl(0.0L) is wrong.
dnl On AIX 7.1 in 64-bit mode, ilogbl(2^(LDBL_MIN_EXP-1)) is wrong.
dnl On Haiku 2017, it returns i-2 instead of i-1 for values between
dnl ca. 2^-16444 and ca. 2^-16382.
AC_DEFUN([gl_FUNC_ILOGBL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether ilogbl works], [gl_cv_func_ilogbl_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
/* Override the values of <float.h>, like done in float.in.h.  */
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
#endif
#if defined __i386__ && (defined __FreeBSD__ || defined __DragonFly__)
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
#endif
#if (defined _ARCH_PPC || defined _POWER) && defined _AIX && (LDBL_MANT_DIG == 106) && defined __GNUC__
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP DBL_MIN_EXP
#endif
#if defined __sgi && (LDBL_MANT_DIG >= 106)
# if defined __GNUC__
#  undef LDBL_MIN_EXP
#  define LDBL_MIN_EXP DBL_MIN_EXP
# endif
#endif
volatile long double x;
static int dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  int (* volatile my_ilogbl) (long double) = argc ? ilogbl : dummy;
  int result = 0;
  /* This test fails on Cygwin 2.9.  */
  {
    x = 0.0L;
    if (my_ilogbl (x) != FP_ILOGB0)
      result |= 1;
  }
  /* This test fails on AIX 7.1 in 64-bit mode.  */
  {
    int i;
    x = 0.5L;
    for (i = LDBL_MIN_EXP - 1; i < 0; i++)
      x = x * 0.5L;
    if (x > 0.0L && my_ilogbl (x) != LDBL_MIN_EXP - 2)
      result |= 2;
  }
  /* This test fails on Haiku 2017.  */
  {
    int i;
    for (i = 1, x = (long double)1.0; i >= LDBL_MIN_EXP-100 && x > (long double)0.0; i--, x *= (long double)0.5)
      if (my_ilogbl (x) != i - 1)
        {
          result |= 4;
          break;
        }
  }
  return result;
}
]])],
        [gl_cv_func_ilogbl_works=yes],
        [gl_cv_func_ilogbl_works=no],
        [case "$host_os" in
           aix* | haiku*)
                   gl_cv_func_ilogbl_works="guessing no" ;;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_ilogbl_works="guessing yes" ;;
           *)      gl_cv_func_ilogbl_works="guessing yes" ;;
         esac
        ])
    ])
])
