# log2l.m4 serial 3
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOG2L],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_LOG2])

  dnl Persuade glibc <math.h> to declare log2l().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether log2l() exists. Assume that log2l(), if it exists, is
  dnl defined in the same library as log2().
  save_LIBS="$LIBS"
  LIBS="$LIBS $LOG2_LIBM"
  AC_CHECK_FUNCS([log2l])
  LIBS="$save_LIBS"
  if test $ac_cv_func_log2l = yes; then
    LOG2L_LIBM="$LOG2_LIBM"
    HAVE_LOG2L=1
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has log2l() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECL([log2l], , [HAVE_DECL_LOG2L=0], [[#include <math.h>]])

    save_LIBS="$LIBS"
    LIBS="$LIBS $LOG2L_LIBM"
    gl_FUNC_LOG2L_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_log2l_works" in
      *yes) ;;
      *) REPLACE_LOG2L=1 ;;
    esac
  else
    HAVE_LOG2L=0
    HAVE_DECL_LOG2L=0
  fi
  if test $HAVE_LOG2L = 0 || test $REPLACE_LOG2L = 1; then
    dnl Find libraries needed to link lib/log2l.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      LOG2L_LIBM="$LOG2_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_LOGL])
      LOG2L_LIBM=
      dnl Append $ISNANL_LIBM to LOG2L_LIBM, avoiding gratuitous duplicates.
      case " $LOG2L_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) LOG2L_LIBM="$LOG2L_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $FREXPL_LIBM to LOG2L_LIBM, avoiding gratuitous duplicates.
      case " $LOG2L_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) LOG2L_LIBM="$LOG2L_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $LOGL_LIBM to LOG2L_LIBM, avoiding gratuitous duplicates.
      case " $LOG2L_LIBM " in
        *" $LOGL_LIBM "*) ;;
        *) LOG2L_LIBM="$LOG2L_LIBM $LOGL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([LOG2L_LIBM])
])

dnl Test whether log2l() works.
dnl On OSF/1 5.1, log2l(-0.0) is NaN.
dnl On musl 1.2.2/{arm64,s390x}, the result is accurate to only 16 digits.
AC_DEFUN([gl_FUNC_LOG2L_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether log2l works], [gl_cv_func_log2l_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
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
#ifndef log2l /* for AIX */
extern
#ifdef __cplusplus
"C"
#endif
long double log2l (long double);
#endif
static long double dummy (long double x) { return 0; }
volatile long double gx;
volatile long double gy;
int main (int argc, char *argv[])
{
  long double (* volatile my_log2l) (long double) = argc ? log2l : dummy;
  int result = 0;
  /* This test fails on OSF/1 5.1.  */
  {
    gx = -0.0L;
    gy = my_log2l (gx);
    if (!(gy + gy == gy))
      result |= 1;
  }
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 11.358L;
    long double y = my_log2l (x);
    long double z = my_log2l (1.0L / x);
    long double err = (y + z) * TWO_LDBL_MANT_DIG;
    if (!(err >= -10000.0L && err <= 10000.0L))
      result |= 2;
  }
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x.  */
  if (DBL_MAX_EXP < LDBL_MAX_EXP)
    {
      long double x = ldexpl (1.0L, DBL_MAX_EXP); /* finite! */
      long double y = my_log2l (x);
      if (y > 0 && y + y == y) /* infinite? */
        result |= 4;
    }
  return result;
}
]])],
        [gl_cv_func_log2l_works=yes],
        [gl_cv_func_log2l_works=no],
        [case "$host_os" in
                          # Guess yes on glibc systems.
           *-gnu* | gnu*) gl_cv_func_log2l_works="guessing yes" ;;
                          # Guess no on musl systems.
           *-musl*)       gl_cv_func_log2l_works="guessing no" ;;
                          # Guess no on OSF/1.
           osf*)          gl_cv_func_log2l_works="guessing no" ;;
                          # Guess yes on native Windows.
           mingw*)        gl_cv_func_log2l_works="guessing yes" ;;
                          # If we don't know, obey --enable-cross-guesses.
           *)             gl_cv_func_log2l_works="$gl_cross_guess_normal" ;;
         esac
        ])
    ])
])
