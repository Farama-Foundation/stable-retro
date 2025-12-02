# fmal.m4 serial 7
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMAL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare fmal().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine FMAL_LIBM.
  gl_MATHFUNC([fmal], [long double], [(long double, long double, long double)],
    [extern
     #ifdef __cplusplus
     "C"
     #endif
     long double fmal (long double, long double, long double);
    ])
  if test $gl_cv_func_fmal_no_libm = yes \
     || test $gl_cv_func_fmal_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has fmal() in libm but doesn't declare it in <math.h>,
    dnl and the function is buggy.
    AC_CHECK_DECL([fmal], , [REPLACE_FMAL=1], [[#include <math.h>]])
    if test $REPLACE_FMAL = 0; then
      gl_FUNC_FMAL_WORKS
      case "$gl_cv_func_fmal_works" in
        *no) REPLACE_FMAL=1 ;;
      esac
    fi
  else
    HAVE_FMAL=0
  fi
  if test $HAVE_FMAL = 0 || test $REPLACE_FMAL = 1; then
    dnl Find libraries needed to link lib/fmal.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_FMA])
      FMAL_LIBM="$FMA_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      AC_REQUIRE([gl_FUNC_FEGETROUND])
      FMAL_LIBM=
      dnl Append $FREXPL_LIBM to FMAL_LIBM, avoiding gratuitous duplicates.
      case " $FMAL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) FMAL_LIBM="$FMAL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to FMAL_LIBM, avoiding gratuitous duplicates.
      case " $FMAL_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) FMAL_LIBM="$FMAL_LIBM $LDEXPL_LIBM" ;;
      esac
      dnl Append $FEGETROUND_LIBM to FMAL_LIBM, avoiding gratuitous duplicates.
      case " $FMAL_LIBM " in
        *" $FEGETROUND_LIBM "*) ;;
        *) FMAL_LIBM="$FMAL_LIBM $FEGETROUND_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([FMAL_LIBM])
])

dnl Test whether fmal() has any of the 15 known bugs of glibc 2.11.3 on x86_64.
AC_DEFUN([gl_FUNC_FMAL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_REQUIRE([gl_FUNC_LDEXPL])
  save_LIBS="$LIBS"
  LIBS="$LIBS $FMAL_LIBM $LDEXPL_LIBM"
  AC_CACHE_CHECK([whether fmal works], [gl_cv_func_fmal_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
/* Override the values of <float.h>, like done in float.in.h.  */
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG   64
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
# undef LDBL_MAX_EXP
# define LDBL_MAX_EXP    16384
#endif
#if defined __i386__ && (defined __FreeBSD__ || defined __DragonFly__)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG   64
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
# undef LDBL_MAX_EXP
# define LDBL_MAX_EXP    16384
#endif
#if (defined _ARCH_PPC || defined _POWER) && defined _AIX && (LDBL_MANT_DIG == 106) && defined __GNUC__
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP DBL_MIN_EXP
#endif
#if defined __sgi && (LDBL_MANT_DIG >= 106)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG 106
# if defined __GNUC__
#  undef LDBL_MIN_EXP
#  define LDBL_MIN_EXP DBL_MIN_EXP
# endif
#endif
long double p0 = 0.0L;
int main()
{
  int failed_tests = 0;
  /* This test fails on glibc 2.11 powerpc.  */
  {
    volatile long double x = 1.5L; /* 3 * 2^-1 */
    volatile long double y = x;
    volatile long double z = ldexpl (1.0L, LDBL_MANT_DIG + 1); /* 2^65 */
    /* x * y + z with infinite precision: 2^65 + 9 * 2^-2.
       Lies between (2^63 + 0) * 2^2 and (2^63 + 1) * 2^2
       and is closer to (2^63 + 1) * 2^2, therefore the rounding
       must round up and produce (2^63 + 1) * 2^2.  */
    volatile long double expected = z + 4.0L;
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 1;
  }
  /* This test fails on glibc 2.11 powerpc.  */
  {
    volatile long double x = 1.25L; /* 2^0 + 2^-2 */
    volatile long double y = - x;
    volatile long double z = ldexpl (1.0L, LDBL_MANT_DIG + 1); /* 2^65 */
    /* x * y + z with infinite precision: 2^65 - 2^0 - 2^-1 - 2^-4.
       Lies between (2^64 - 1) * 2^1 and 2^64 * 2^1
       and is closer to (2^64 - 1) * 2^1, therefore the rounding
       must round down and produce (2^64 - 1) * 2^1.  */
    volatile long double expected = (ldexpl (1.0L, LDBL_MANT_DIG) - 1.0L) * 2.0L;
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 1;
  }
  /* This test fails on glibc 2.11 x86,x86_64,powerpc, glibc 2.7 hppa,sparc,
     OSF/1 5.1, mingw.  */
  {
    volatile long double x = 1.0L + ldexpl (1.0L, 1 - LDBL_MANT_DIG); /* 2^0 + 2^-63 */
    volatile long double y = x;
    volatile long double z = 4.0L; /* 2^2 */
    /* x * y + z with infinite precision: 2^2 + 2^0 + 2^-62 + 2^-126.
       Lies between (2^63 + 2^61) * 2^-61 and (2^63 + 2^61 + 1) * 2^-61
       and is closer to (2^63 + 2^61 + 1) * 2^-61, therefore the rounding
       must round up and produce (2^63 + 2^61 + 1) * 2^-61.  */
    volatile long double expected = 4.0L + 1.0L + ldexpl (1.0L, 3 - LDBL_MANT_DIG);
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 2;
  }
  /* This test fails on glibc 2.11 x86,x86_64,powerpc glibc 2.7 hppa,sparc,
     OSF/1 5.1, mingw.  */
  {
    volatile long double x = 1.0L + ldexpl (1.0L, 1 - LDBL_MANT_DIG); /* 2^0 + 2^-63 */
    volatile long double y = - x;
    volatile long double z = 8.0L; /* 2^3 */
    /* x * y + z with infinite precision: 2^2 + 2^1 + 2^0 - 2^-62 - 2^-126.
       Lies between (2^63 + 2^62 + 2^61 - 1) * 2^-61 and
       (2^63 + 2^62 + 2^61) * 2^-61 and is closer to
       (2^63 + 2^62 + 2^61 - 1) * 2^-61, therefore the rounding
       must round down and produce (2^63 + 2^62 + 2^61 - 1) * 2^-61.  */
    volatile long double expected = 7.0L - ldexpl (1.0L, 3 - LDBL_MANT_DIG);
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 2;
  }
  /* This test fails on glibc 2.11 powerpc.  */
  {
    volatile long double x = 1.25L; /* 2^0 + 2^-2 */
    volatile long double y = - 0.75L; /* - 2^0 + 2^-2 */
    volatile long double z = ldexpl (1.0L, LDBL_MANT_DIG); /* 2^64 */
    /* x * y + z with infinite precision: 2^64 - 2^0 + 2^-4.
       Lies between (2^64 - 2^0) and 2^64 and is closer to (2^64 - 2^0),
       therefore the rounding must round down and produce (2^64 - 2^0).  */
    volatile long double expected = ldexpl (1.0L, LDBL_MANT_DIG) - 1.0L;
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 1;
  }
  if ((LDBL_MANT_DIG % 2) == 1)
    {
      /* These tests fail on glibc 2.7 hppa,sparc, OSF/1 5.1.  */
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - (LDBL_MANT_DIG + 1) / 2); /* 2^0 + 2^-27 */
        volatile long double y = 1.0L - ldexpl (1.0L, - (LDBL_MANT_DIG + 1) / 2); /* 2^0 - 2^-27 */
        volatile long double z = - ldexpl (1.0L, LDBL_MIN_EXP - LDBL_MANT_DIG); /* - 2^-1074 */
        /* x * y + z with infinite precision: 2^0 - 2^-54 - 2^-1074.
           Lies between (2^53 - 1) * 2^-53 and 2^53 * 2^-53 and is closer to
           (2^53 - 1) * 2^-53, therefore the rounding must round down and
           produce (2^53 - 1) * 2^-53.  */
        volatile long double expected = 1.0L - ldexpl (1.0L, - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 4;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - (LDBL_MANT_DIG + 1) / 2); /* 2^0 + 2^-57 */
        volatile long double y = x;
        volatile long double z = ldexpl (1.0L, - LDBL_MANT_DIG); /* 2^-113 */
        /* x * y + z with infinite precision: 2^0 + 2^-56 + 2^-113 + 2^-114.
           Lies between (2^112 + 2^56) * 2^-112 and (2^112 + 2^56 + 1) * 2^-112
           and is closer to (2^112 + 2^56 + 1) * 2^-112, therefore the rounding
           must round up and produce (2^112 + 2^56 + 1) * 2^-112.  */
        volatile long double expected =
          1.0L + ldexpl (1.0L, - (LDBL_MANT_DIG - 1) / 2) + ldexpl (1.0L, 1 - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 4;
      }
    }
  else
    {
      /* These tests fail on glibc 2.11 x86,x86_64,powerpc, mingw.  */
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = ldexpl (1.0L, LDBL_MIN_EXP - LDBL_MANT_DIG); /* 2^-16445 */
        /* x * y + z with infinite precision: 2^0 + 2^-31 + 2^-64 + 2^-16445.
           Lies between (2^63 + 2^32 + 0) * 2^-63 and (2^63 + 2^32 + 1) * 2^-63
           and is closer to (2^63 + 2^32 + 1) * 2^-63, therefore the rounding
           must round up and produce (2^63 + 2^32 + 1) * 2^-63.  */
        volatile long double expected =
          1.0L + ldexpl (1.0L, 1 - LDBL_MANT_DIG / 2) + ldexpl (1.0L, 1 - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = ldexpl (1.0L, - LDBL_MANT_DIG); /* 2^-64 */
        /* x * y + z with infinite precision: 2^0 + 2^-31 + 2^-63.
           Rounding must return this value unchanged.  */
        volatile long double expected = 1.0L + ldexpl (1.0L, 1 - LDBL_MANT_DIG / 2) + ldexpl (1.0L, 1 - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = ldexpl (1.0L, 1 - LDBL_MANT_DIG); /* 2^-63 */
        /* x * y + z with infinite precision: 2^0 + 2^-31 + 2^-63 + 2^-64.
           Lies between (2^63 + 2^32 + 1) * 2^-63 and (2^63 + 2^32 + 2) * 2^-63
           and is at the same distance from each.  According to the round-to-even
           rule, the rounding must round up and produce (2^63 + 2^32 + 2) * 2^-63.  */
        volatile long double expected = 1.0L + ldexpl (1.0L, -31) + ldexpl (1.0L, -62);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = ldexpl (1.0L, LDBL_MANT_DIG / 2 + 1); /* 2^33 */
        /* x * y + z with infinite precision: 2^33 + 2^0 + 2^-31 + 2^-64.
           Lies between (2^63 + 2^30) * 2^-30 and (2^63 + 2^30 + 1) * 2^-30
           and is closer to (2^63 + 2^30 + 1) * 2^-30, therefore the rounding
           must round up and produce (2^63 + 2^30 + 1) * 2^-30.  */
        volatile long double expected = z + 1.0L + ldexp (1.0L, 2 - LDBL_MANT_DIG / 2);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = - ldexpl (1.0, 1 - LDBL_MANT_DIG); /* - 2^-63 */
        /* x * y + z with infinite precision: 2^0 + 2^-31 - 2^-64.
           Lies between (2^63 + 2^32 - 1) * 2^-63 and (2^63 + 2^32) * 2^-63
           and is at the same distance from each.  According to the round-to-even
           rule, the rounding must round up and produce (2^63 + 2^32) * 2^-63.  */
        volatile long double expected = 1.0L + ldexpl (1.0L, 1 - LDBL_MANT_DIG / 2);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = x;
        volatile long double z = - 1.0L; /* - 2^0 */
        /* x * y + z with infinite precision: 2^-31 + 2^-64.
           Rounding must return this value unchanged.  */
        volatile long double expected = ldexpl (1.0L, 1 - LDBL_MANT_DIG / 2) + ldexpl (1.0L, - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = - x;
        volatile long double z = 2.0L; /* 2^1 */
        /* x * y + z with infinite precision: 2^0 - 2^31 - 2^-64.
           Rounding must return this value unchanged.  */
        volatile long double expected = 1.0L - ldexpl (1.0L, 1 - LDBL_MANT_DIG / 2) - ldexpl (1.0L, - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2); /* 2^0 + 2^-32 */
        volatile long double y = - x;
        volatile long double z = ldexpl (1.0L, LDBL_MANT_DIG / 2 + 2); /* 2^34 */
        /* x * y + z with infinite precision: 2^34 - (2^0 + 2^-31 + 2^-64).
           Lies between (2^64 - 2^30 - 1) * 2^-30 and (2^64 - 2^30) * 2^-30
           and is closer to (2^64 - 2^30 - 1) * 2^-30, therefore the rounding
           must round down and produce (2^64 - 2^30 - 1) * 2^-30.  */
        volatile long double expected = z - 1.0L - ldexpl (1.0L, 2 - LDBL_MANT_DIG / 2);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2 - 1); /* 2^0 + 2^-33 */
        volatile long double y = 1.0L - ldexpl (1.0L, - LDBL_MANT_DIG / 2 - 1); /* 2^0 - 2^-33 */
        volatile long double z = - ldexpl (1.0L, - LDBL_MANT_DIG - 1); /* 2^-65 */
        /* x * y + z with infinite precision: 2^0 - 2^-65 - 2^-66.
           Lies between (2^64 - 1) * 2^-64 and 2^64 * 2^-64 and is closer to
           (2^64 - 1) * 2^-64, therefore the rounding must round down and
           produce (2^64 - 1) * 2^-64.  */
        volatile long double expected = 1.0L - ldexpl (1.0L, - LDBL_MANT_DIG);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
      {
        volatile long double x = 1.0L + ldexpl (1.0L, - LDBL_MANT_DIG / 2 - 1); /* 2^0 + 2^-33 */
        volatile long double y = 1.0L - ldexpl (1.0L, - LDBL_MANT_DIG / 2 - 1); /* 2^0 - 2^-33 */
        volatile long double z = - 1.0L; /* 2^0 */
        /* x * y + z with infinite precision: - 2^-66.
           Rounding must return this value unchanged.  */
        volatile long double expected = - ldexpl (1.0L, - LDBL_MANT_DIG - 2);
        volatile long double result = fmal (x, y, z);
        if (result != expected)
          failed_tests |= 8;
      }
    }
  /* This test fails on glibc 2.11 x86,x86_64,powerpc, glibc 2.7 hppa,sparc,
     FreeBSD 6.4 x86, mingw.  */
  {
    long double minus_inf = -1.0L / p0;
    volatile long double x = ldexpl (1.0L, LDBL_MAX_EXP - 1);
    volatile long double y = ldexpl (1.0L, LDBL_MAX_EXP - 1);
    volatile long double z = minus_inf;
    volatile long double result = fmal (x, y, z);
    if (!(result == minus_inf))
      failed_tests |= 16;
  }
  /* This test fails on glibc 2.11 x86,x86_64,powerpc glibc 2.7 hppa,sparc,
     Mac OS X 10.5, FreeBSD 6.4 x86, OSF/1 5.1, mingw.  */
  {
    volatile long double x = ldexpl (1.0L, LDBL_MAX_EXP - 1);
    volatile long double y = 2.0L;
    volatile long double z =
      - ldexpl (ldexpl (1.0L, LDBL_MAX_EXP - 1) - ldexpl (1.0L, LDBL_MAX_EXP - LDBL_MANT_DIG - 1), 1);
    volatile long double expected = ldexpl (1.0L, LDBL_MAX_EXP - LDBL_MANT_DIG);
    volatile long double result = fmal (x, y, z);
    if (result != expected)
      failed_tests |= 32;
  }
  return failed_tests;
}]])],
        [gl_cv_func_fmal_works=yes],
        [gl_cv_func_fmal_works=no],
        [dnl Guess yes on native Windows with MSVC.
         dnl Otherwise guess no, even on glibc systems.
         gl_cv_func_fmal_works="$gl_cross_guess_normal"
         case "$host_os" in
           mingw*)
             AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
               ], [gl_cv_func_fmal_works="guessing yes"])
             ;;
         esac
        ])
    ])
  LIBS="$save_LIBS"
])

# Prerequisites of lib/fmal.c.
AC_DEFUN([gl_PREREQ_FMAL], [:])
