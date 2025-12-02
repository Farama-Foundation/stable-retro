# logbl.m4 serial 5
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOGBL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_LOGB])

  dnl Persuade glibc <math.h> to declare logbl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether logbl() exists and determine LOGBL_LIBM. We cannot assume
  dnl that logbl(), if it exists, is defined in the same library as logb().
  dnl This is not the case on AIX >= 5.2, Solaris >= 10.
  gl_MATHFUNC([logbl], [long double], [(long double)])
  if test $gl_cv_func_logbl_no_libm = yes \
     || test $gl_cv_func_logbl_in_libm = yes; then
    save_LIBS="$LIBS"
    LIBS="$LIBS $LOGBL_LIBM"
    gl_FUNC_LOGBL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_logbl_works" in
      *yes) ;;
      *) REPLACE_LOGBL=1 ;;
    esac
  else
    HAVE_LOGBL=0
  fi
  if test $HAVE_LOGBL = 0 || test $REPLACE_LOGBL = 1; then
    dnl Find libraries needed to link lib/logbl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      LOGBL_LIBM="$LOGB_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_ISNANL])
      LOGBL_LIBM=
      dnl Append $FREXPL_LIBM to LOGBL_LIBM, avoiding gratuitous duplicates.
      case " $LOGBL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) LOGBL_LIBM="$LOGBL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $ISNANL_LIBM to LOGBL_LIBM, avoiding gratuitous duplicates.
      case " $LOGBL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) LOGBL_LIBM="$LOGBL_LIBM $ISNANL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([LOGBL_LIBM])
])

dnl Test whether logbl() works.
dnl On glibc 2.11/powerpc, glibc 2.7/sparc, glibc 2.7/hppa, Solaris 10/SPARC,
dnl Solaris 11.4/x86_64, the return value for subnormal (denormalized) arguments
dnl is too large.
dnl On glibc 2.23/powerpc64le, the return value for negative subnormal arguments
dnl is too large.
AC_DEFUN([gl_FUNC_LOGBL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether logbl works], [gl_cv_func_logbl_works],
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
int main ()
{
  int i;
  for (i = 1, x = 1.0L; i >= LDBL_MIN_EXP - 54; i--, x *= 0.5L)
    /* Either x = 2^(i-1) or x = 0.0.  */
    if ((i == LDBL_MIN_EXP - 1 || i == LDBL_MIN_EXP - 54)
        && (x > 0.0L && !(logbl (x) == (long double)(i - 1))))
      return 1;
  for (i = 1, x = -1.0L; i >= LDBL_MIN_EXP - 54; i--, x *= 0.5L)
    /* Either x = -2^(i-1) or x = 0.0.  */
    if ((i == LDBL_MIN_EXP - 1 || i == LDBL_MIN_EXP - 54)
        && (x < 0.0L && !(logbl (x) == (long double)(i - 1))))
      return 1;
  return 0;
}
]])],
        [gl_cv_func_logbl_works=yes],
        [gl_cv_func_logbl_works=no],
        [case "$host_os" in
           *gnu* | solaris*) gl_cv_func_logbl_works="guessing no" ;;
                             # Guess yes on native Windows.
           mingw*)           gl_cv_func_logbl_works="guessing yes" ;;
           *)                gl_cv_func_logbl_works="guessing yes" ;;
         esac
        ])
    ])
])
