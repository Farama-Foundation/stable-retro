# expl.m4 serial 18
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXPL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc <math.h> to declare expl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  EXPL_LIBM=
  AC_CACHE_CHECK([whether expl() can be used without linking with libm],
    [gl_cv_func_expl_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             long double (* volatile funcptr) (long double) = expl;
             long double x;]],
           [[return funcptr (x) > 1.5
                    || expl (x) > 1.5;]])],
        [gl_cv_func_expl_no_libm=yes],
        [gl_cv_func_expl_no_libm=no])
    ])
  if test $gl_cv_func_expl_no_libm = no; then
    AC_CACHE_CHECK([whether expl() can be used with libm],
      [gl_cv_func_expl_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               long double (* volatile funcptr) (long double) = expl;
               long double x;]],
             [[return funcptr (x) > 1.5
                      || expl (x) > 1.5;]])],
          [gl_cv_func_expl_in_libm=yes],
          [gl_cv_func_expl_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_expl_in_libm = yes; then
      EXPL_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_expl_no_libm = yes \
     || test $gl_cv_func_expl_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl Mac OS X 10.3 has expl() in libc but doesn't declare it in <math.h>.
    AC_CHECK_DECL([expl], , [HAVE_DECL_EXPL=0], [[#include <math.h>]])
    if test $REPLACE_EXPL = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether expl works],
        [gl_cv_func_expl_works],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $EXPL_LIBM"
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
#undef expl
extern
#ifdef __cplusplus
"C"
#endif
long double expl (long double);
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_expl) (long double) = argc ? expl : dummy;
  int result = 0;
  /* On Haiku 2017 the system's native expl() is just a stub: it returns 0.0
     and prints "__expl not implemented" for all arguments.  */
  {
    volatile long double x1 = -1.0;
    if (expl (x1) == 0.0)
      result |= 1;
  }
  /* On OpenBSD 5.4 the system's native expl() is buggy:
     it returns 'nan' for small values.  */
  {
    volatile long double x1 = -1.0;
    volatile long double x2 = -0.8;
    volatile long double x3 = -0.4;
    if (isnan (expl (x1)) || isnan (expl (x2)) || isnan (expl (x3)))
      result |= 2;
  }
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x, NetBSD 9.0.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 11.358L;
    long double err = (my_expl (x) * my_expl (- x) - 1.0L) * TWO_LDBL_MANT_DIG;
    if (!(err >= -100.0L && err <= 100.0L))
      result |= 4;
  }
  return result;
}
            ]])],
            [gl_cv_func_expl_works=yes],
            [gl_cv_func_expl_works=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_expl_works="guessing yes" ;;
                              # Guess no on musl systems.
               *-musl*)       gl_cv_func_expl_works="guessing no" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_expl_works="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_expl_works="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_expl_works" in
        *yes) ;;
        *) REPLACE_EXPL=1 ;;
      esac
    fi
  else
    HAVE_DECL_EXPL=0
    HAVE_EXPL=0
  fi
  if test $HAVE_EXPL = 0 || test $REPLACE_EXPL = 1; then
    dnl Find libraries needed to link lib/expl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_EXP])
      EXPL_LIBM="$EXP_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_ROUNDL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      EXPL_LIBM=
      dnl Append $ISNANL_LIBM to EXPL_LIBM, avoiding gratuitous duplicates.
      case " $EXPL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) EXPL_LIBM="$EXPL_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $ROUNDL_LIBM to EXPL_LIBM, avoiding gratuitous duplicates.
      case " $EXPL_LIBM " in
        *" $ROUNDL_LIBM "*) ;;
        *) EXPL_LIBM="$EXPL_LIBM $ROUNDL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to EXPL_LIBM, avoiding gratuitous duplicates.
      case " $EXPL_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) EXPL_LIBM="$EXPL_LIBM $LDEXPL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([EXPL_LIBM])
])
