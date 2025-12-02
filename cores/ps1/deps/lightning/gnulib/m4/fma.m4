# fma.m4 serial 4
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMA],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Determine FMA_LIBM.
  gl_MATHFUNC([fma], [double], [(double, double, double)],
    [extern
     #ifdef __cplusplus
     "C"
     #endif
     double fma (double, double, double);
    ])
  if test $gl_cv_func_fma_no_libm = yes \
     || test $gl_cv_func_fma_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has fma() in libm but doesn't declare it in <math.h>,
    dnl and the function is buggy.
    AC_CHECK_DECL([fma], , [REPLACE_FMA=1], [[#include <math.h>]])
    if test $REPLACE_FMA = 0; then
      gl_FUNC_FMA_WORKS
      case "$gl_cv_func_fma_works" in
        *no) REPLACE_FMA=1 ;;
      esac
    fi
  else
    HAVE_FMA=0
  fi
  if test $HAVE_FMA = 0 || test $REPLACE_FMA = 1; then
    dnl Find libraries needed to link lib/fmal.c.
    AC_REQUIRE([gl_FUNC_FREXP])
    AC_REQUIRE([gl_FUNC_LDEXP])
    AC_REQUIRE([gl_FUNC_FEGETROUND])
    FMA_LIBM=
    dnl Append $FREXP_LIBM to FMA_LIBM, avoiding gratuitous duplicates.
    case " $FMA_LIBM " in
      *" $FREXP_LIBM "*) ;;
      *) FMA_LIBM="$FMA_LIBM $FREXP_LIBM" ;;
    esac
    dnl Append $LDEXP_LIBM to FMA_LIBM, avoiding gratuitous duplicates.
    case " $FMA_LIBM " in
      *" $LDEXP_LIBM "*) ;;
      *) FMA_LIBM="$FMA_LIBM $LDEXP_LIBM" ;;
    esac
    dnl Append $FEGETROUND_LIBM to FMA_LIBM, avoiding gratuitous duplicates.
    case " $FMA_LIBM " in
      *" $FEGETROUND_LIBM "*) ;;
      *) FMA_LIBM="$FMA_LIBM $FEGETROUND_LIBM" ;;
    esac
  fi
  AC_SUBST([FMA_LIBM])
])

dnl Test whether fma() has any of the 7 known bugs of glibc 2.11.3 on x86_64.
AC_DEFUN([gl_FUNC_FMA_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_REQUIRE([gl_FUNC_LDEXP])
  save_LIBS="$LIBS"
  LIBS="$LIBS $FMA_LIBM $LDEXP_LIBM"
  AC_CACHE_CHECK([whether fma works], [gl_cv_func_fma_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
double p0 = 0.0;
int main()
{
  int failed_tests = 0;
  /* These tests fail with glibc 2.11.3 on x86_64.  */
  {
    volatile double x = 1.5; /* 3 * 2^-1 */
    volatile double y = x;
    volatile double z = ldexp (1.0, DBL_MANT_DIG + 1); /* 2^54 */
    /* x * y + z with infinite precision: 2^54 + 9 * 2^-2.
       Lies between (2^52 + 0) * 2^2 and (2^52 + 1) * 2^2
       and is closer to (2^52 + 1) * 2^2, therefore the rounding
       must round up and produce (2^52 + 1) * 2^2.  */
    volatile double expected = z + 4.0;
    volatile double result = fma (x, y, z);
    if (result != expected)
      failed_tests |= 1;
  }
  {
    volatile double x = 1.25; /* 2^0 + 2^-2 */
    volatile double y = - x;
    volatile double z = ldexp (1.0, DBL_MANT_DIG + 1); /* 2^54 */
    /* x * y + z with infinite precision: 2^54 - 2^0 - 2^-1 - 2^-4.
       Lies between (2^53 - 1) * 2^1 and 2^53 * 2^1
       and is closer to (2^53 - 1) * 2^1, therefore the rounding
       must round down and produce (2^53 - 1) * 2^1.  */
    volatile double expected = (ldexp (1.0, DBL_MANT_DIG) - 1.0) * 2.0;
    volatile double result = fma (x, y, z);
    if (result != expected)
      failed_tests |= 2;
  }
  {
    volatile double x = 1.0 + ldexp (1.0, 1 - DBL_MANT_DIG); /* 2^0 + 2^-52 */
    volatile double y = x;
    volatile double z = 4.0; /* 2^2 */
    /* x * y + z with infinite precision: 2^2 + 2^0 + 2^-51 + 2^-104.
       Lies between (2^52 + 2^50) * 2^-50 and (2^52 + 2^50 + 1) * 2^-50
       and is closer to (2^52 + 2^50 + 1) * 2^-50, therefore the rounding
       must round up and produce (2^52 + 2^50 + 1) * 2^-50.  */
    volatile double expected = 4.0 + 1.0 + ldexp (1.0, 3 - DBL_MANT_DIG);
    volatile double result = fma (x, y, z);
    if (result != expected)
      failed_tests |= 4;
  }
  {
    volatile double x = 1.0 + ldexp (1.0, 1 - DBL_MANT_DIG); /* 2^0 + 2^-52 */
    volatile double y = - x;
    volatile double z = 8.0; /* 2^3 */
    /* x * y + z with infinite precision: 2^2 + 2^1 + 2^0 - 2^-51 - 2^-104.
       Lies between (2^52 + 2^51 + 2^50 - 1) * 2^-50 and
       (2^52 + 2^51 + 2^50) * 2^-50 and is closer to
       (2^52 + 2^51 + 2^50 - 1) * 2^-50, therefore the rounding
       must round down and produce (2^52 + 2^51 + 2^50 - 1) * 2^-50.  */
    volatile double expected = 7.0 - ldexp (1.0, 3 - DBL_MANT_DIG);
    volatile double result = fma (x, y, z);
    if (result != expected)
      failed_tests |= 8;
  }
  {
    volatile double x = 1.25; /* 2^0 + 2^-2 */
    volatile double y = - 0.75; /* - 2^0 + 2^-2 */
    volatile double z = ldexp (1.0, DBL_MANT_DIG); /* 2^53 */
    /* x * y + z with infinite precision: 2^53 - 2^0 + 2^-4.
       Lies between (2^53 - 2^0) and 2^53 and is closer to (2^53 - 2^0),
       therefore the rounding must round down and produce (2^53 - 2^0).  */
    volatile double expected = ldexp (1.0, DBL_MANT_DIG) - 1.0;
    volatile double result = fma (x, y, z);
    if (result != expected)
      failed_tests |= 16;
  }
  if ((DBL_MANT_DIG % 2) == 1)
    {
      volatile double x = 1.0 + ldexp (1.0, - (DBL_MANT_DIG + 1) / 2); /* 2^0 + 2^-27 */
      volatile double y = 1.0 - ldexp (1.0, - (DBL_MANT_DIG + 1) / 2); /* 2^0 - 2^-27 */
      volatile double z = - ldexp (1.0, DBL_MIN_EXP - DBL_MANT_DIG); /* - 2^-1074 */
      /* x * y + z with infinite precision: 2^0 - 2^-54 - 2^-1074.
         Lies between (2^53 - 1) * 2^-53 and 2^53 * 2^-53 and is closer to
         (2^53 - 1) * 2^-53, therefore the rounding must round down and
         produce (2^53 - 1) * 2^-53.  */
      volatile double expected = 1.0 - ldexp (1.0, - DBL_MANT_DIG);
      volatile double result = fma (x, y, z);
      if (result != expected)
        failed_tests |= 32;
    }
  {
    double minus_inf = -1.0 / p0;
    volatile double x = ldexp (1.0, DBL_MAX_EXP - 1);
    volatile double y = ldexp (1.0, DBL_MAX_EXP - 1);
    volatile double z = minus_inf;
    volatile double result = fma (x, y, z);
    if (!(result == minus_inf))
      failed_tests |= 64;
  }
  return failed_tests;
}]])],
        [gl_cv_func_fma_works=yes],
        [gl_cv_func_fma_works=no],
        [dnl Guess yes on native Windows with MSVC.
         dnl Otherwise guess no, even on glibc systems.
         gl_cv_func_fma_works="$gl_cross_guess_normal"
         case "$host_os" in
           mingw*)
             AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
               ], [gl_cv_func_fma_works="guessing yes"])
             ;;
         esac
        ])
    ])
  LIBS="$save_LIBS"
])

# Prerequisites of lib/fma.c.
AC_DEFUN([gl_PREREQ_FMA], [:])
