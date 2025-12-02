# fmaf.m4 serial 6
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMAF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare fmaf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine FMAF_LIBM.
  gl_MATHFUNC([fmaf], [float], [(float, float, float)],
    [extern
     #ifdef __cplusplus
     "C"
     #endif
     float fmaf (float, float, float);
    ])
  if test $gl_cv_func_fmaf_no_libm = yes \
     || test $gl_cv_func_fmaf_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has fmaf() in libm but doesn't declare it in <math.h>,
    dnl and the function is likely buggy.
    AC_CHECK_DECL([fmaf], , [REPLACE_FMAF=1], [[#include <math.h>]])
    if test $REPLACE_FMAF = 0; then
      gl_FUNC_FMAF_WORKS
      case "$gl_cv_func_fmaf_works" in
        *no) REPLACE_FMAF=1 ;;
      esac
    fi
  else
    HAVE_FMAF=0
  fi
  if test $HAVE_FMAF = 0 || test $REPLACE_FMAF = 1; then
    dnl Find libraries needed to link lib/fmaf.c.
    AC_REQUIRE([gl_FUNC_FREXPF])
    AC_REQUIRE([gl_FUNC_LDEXPF])
    AC_REQUIRE([gl_FUNC_FEGETROUND])
    FMAF_LIBM=
    dnl Append $FREXPF_LIBM to FMAF_LIBM, avoiding gratuitous duplicates.
    case " $FMAF_LIBM " in
      *" $FREXPF_LIBM "*) ;;
      *) FMAF_LIBM="$FMAF_LIBM $FREXPF_LIBM" ;;
    esac
    dnl Append $LDEXPF_LIBM to FMAF_LIBM, avoiding gratuitous duplicates.
    case " $FMAF_LIBM " in
      *" $LDEXPF_LIBM "*) ;;
      *) FMAF_LIBM="$FMAF_LIBM $LDEXPF_LIBM" ;;
    esac
    dnl Append $FEGETROUND_LIBM to FMAF_LIBM, avoiding gratuitous duplicates.
    case " $FMAF_LIBM " in
      *" $FEGETROUND_LIBM "*) ;;
      *) FMAF_LIBM="$FMAF_LIBM $FEGETROUND_LIBM" ;;
    esac
  fi
  AC_SUBST([FMAF_LIBM])
])

dnl Test whether fmaf() has any of the 7 known bugs of glibc 2.11.3 on x86_64.
AC_DEFUN([gl_FUNC_FMAF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_REQUIRE([gl_FUNC_LDEXPF])
  save_LIBS="$LIBS"
  LIBS="$LIBS $FMAF_LIBM $LDEXPF_LIBM"
  AC_CACHE_CHECK([whether fmaf works], [gl_cv_func_fmaf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
float p0 = 0.0f;
int main()
{
  int failed_tests = 0;
  /* These tests fail with glibc 2.11.3 on x86_64.  */
  {
    volatile float x = 1.5f; /* 3 * 2^-1 */
    volatile float y = x;
    volatile float z = ldexpf (1.0f, FLT_MANT_DIG + 1); /* 2^25 */
    /* x * y + z with infinite precision: 2^25 + 9 * 2^-2.
       Lies between (2^23 + 0) * 2^2 and (2^23 + 1) * 2^2
       and is closer to (2^23 + 1) * 2^2, therefore the rounding
       must round up and produce (2^23 + 1) * 2^2.  */
    volatile float expected = z + 4.0f;
    volatile float result = fmaf (x, y, z);
    if (result != expected)
      failed_tests |= 1;
  }
  {
    volatile float x = 1.25f; /* 2^0 + 2^-2 */
    volatile float y = - x;
    volatile float z = ldexpf (1.0f, FLT_MANT_DIG + 1); /* 2^25 */
    /* x * y + z with infinite precision: 2^25 - 2^0 - 2^-1 - 2^-4.
       Lies between (2^24 - 1) * 2^1 and 2^24 * 2^1
       and is closer to (2^24 - 1) * 2^1, therefore the rounding
       must round down and produce (2^24 - 1) * 2^1.  */
    volatile float expected = (ldexpf (1.0f, FLT_MANT_DIG) - 1.0f) * 2.0f;
    volatile float result = fmaf (x, y, z);
    if (result != expected)
      failed_tests |= 2;
  }
  {
    volatile float x = 1.0f + ldexpf (1.0f, 1 - FLT_MANT_DIG); /* 2^0 + 2^-23 */
    volatile float y = x;
    volatile float z = 4.0f; /* 2^2 */
    /* x * y + z with infinite precision: 2^2 + 2^0 + 2^-22 + 2^-46.
       Lies between (2^23 + 2^21) * 2^-21 and (2^23 + 2^21 + 1) * 2^-21
       and is closer to (2^23 + 2^21 + 1) * 2^-21, therefore the rounding
       must round up and produce (2^23 + 2^21 + 1) * 2^-21.  */
    volatile float expected = 4.0f + 1.0f + ldexpf (1.0f, 3 - FLT_MANT_DIG);
    volatile float result = fmaf (x, y, z);
    if (result != expected)
      failed_tests |= 4;
  }
  {
    volatile float x = 1.0f + ldexpf (1.0f, 1 - FLT_MANT_DIG); /* 2^0 + 2^-23 */
    volatile float y = - x;
    volatile float z = 8.0f; /* 2^3 */
    /* x * y + z with infinite precision: 2^2 + 2^1 + 2^0 - 2^-22 - 2^-46.
       Lies between (2^23 + 2^22 + 2^21 - 1) * 2^-21 and
       (2^23 + 2^22 + 2^21) * 2^-21 and is closer to
       (2^23 + 2^22 + 2^21 - 1) * 2^-21, therefore the rounding
       must round down and produce (2^23 + 2^22 + 2^21 - 1) * 2^-21.  */
    volatile float expected = 7.0f - ldexpf (1.0f, 3 - FLT_MANT_DIG);
    volatile float result = fmaf (x, y, z);
    if (result != expected)
      failed_tests |= 8;
  }
  {
    volatile float x = 1.25f; /* 2^0 + 2^-2 */
    volatile float y = - 0.75f; /* - 2^0 + 2^-2 */
    volatile float z = ldexpf (1.0f, FLT_MANT_DIG); /* 2^24 */
    /* x * y + z with infinite precision: 2^24 - 2^0 + 2^-4.
       Lies between (2^24 - 2^0) and 2^24 and is closer to (2^24 - 2^0),
       therefore the rounding must round down and produce (2^24 - 2^0).  */
    volatile float expected = ldexpf (1.0f, FLT_MANT_DIG) - 1.0f;
    volatile float result = fmaf (x, y, z);
    if (result != expected)
      failed_tests |= 16;
  }
  if ((FLT_MANT_DIG % 2) == 0)
    {
      volatile float x = 1.0f + ldexpf (1.0f, - FLT_MANT_DIG / 2); /* 2^0 + 2^-12 */
      volatile float y = x;
      volatile float z = ldexpf (1.0f, FLT_MIN_EXP - FLT_MANT_DIG); /* 2^-149 */
      /* x * y + z with infinite precision: 2^0 + 2^-11 + 2^-24 + 2^-149.
         Lies between (2^23 + 2^12 + 0) * 2^-23 and (2^23 + 2^12 + 1) * 2^-23
         and is closer to (2^23 + 2^12 + 1) * 2^-23, therefore the rounding
         must round up and produce (2^23 + 2^12 + 1) * 2^-23.  */
      volatile float expected =
        1.0f + ldexpf (1.0f, 1 - FLT_MANT_DIG / 2) + ldexpf (1.0f, 1 - FLT_MANT_DIG);
      volatile float result = fmaf (x, y, z);
      if (result != expected)
        failed_tests |= 32;
    }
  {
    float minus_inf = -1.0f / p0;
    volatile float x = ldexpf (1.0f, FLT_MAX_EXP - 1);
    volatile float y = ldexpf (1.0f, FLT_MAX_EXP - 1);
    volatile float z = minus_inf;
    volatile float result = fmaf (x, y, z);
    if (!(result == minus_inf))
      failed_tests |= 64;
  }
  /* This test fails on FreeBSD 12.2/arm.  */
  if ((FLT_MANT_DIG % 2) == 0)
    {
      volatile float x = 1.0f + ldexpf (1.0f, - FLT_MANT_DIG / 2); /* 2^0 + 2^-12 */
      volatile float y = x;
      volatile float z = - ldexpf (1.0f, FLT_MIN_EXP - FLT_MANT_DIG); /* 2^-149 */
      /* x * y + z with infinite precision: 2^0 + 2^-11 + 2^-24 - 2^-149.
         Lies between (2^23 + 2^12 + 0) * 2^-23 and (2^23 + 2^12 + 1) * 2^-23
         and is closer to (2^23 + 2^12 + 0) * 2^-23, therefore the rounding
         must round down and produce (2^23 + 2^12 + 0) * 2^-23.  */
      volatile float expected = 1.0f + ldexpf (1.0f, 1 - FLT_MANT_DIG / 2);
      volatile float result = fmaf (x, y, z);
      if (result != expected)
        failed_tests |= 32;
    }
  return failed_tests;
}]])],
        [gl_cv_func_fmaf_works=yes],
        [gl_cv_func_fmaf_works=no],
        [dnl Guess yes on native Windows with MSVC.
         dnl Otherwise guess no, even on glibc systems.
         gl_cv_func_fmaf_works="$gl_cross_guess_normal"
         case "$host_os" in
           mingw*)
             AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
               ], [gl_cv_func_fmaf_works="guessing yes"])
             ;;
         esac
        ])
    ])
  LIBS="$save_LIBS"
])

# Prerequisites of lib/fmaf.c.
AC_DEFUN([gl_PREREQ_FMAF], [:])
