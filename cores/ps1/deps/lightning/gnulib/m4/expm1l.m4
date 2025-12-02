# expm1l.m4 serial 9
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXPM1L],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare expm1l().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  EXPM1L_LIBM=
  AC_CACHE_CHECK([whether expm1l() can be used without linking with libm],
    [gl_cv_func_expm1l_no_libm],
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
             long double expm1l (long double);
             long double (* volatile funcptr) (long double) = expm1l;
             long double x;]],
           [[return funcptr (x) > 0.5
                    || expm1l (x) > 0.5;]])],
        [gl_cv_func_expm1l_no_libm=yes],
        [gl_cv_func_expm1l_no_libm=no])
    ])
  if test $gl_cv_func_expm1l_no_libm = no; then
    AC_CACHE_CHECK([whether expm1l() can be used with libm],
      [gl_cv_func_expm1l_in_libm],
      [
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
               long double expm1l (long double);
               long double (* volatile funcptr) (long double) = expm1l;
               long double x;]],
             [[return funcptr (x) > 0.5
                      || expm1l (x) > 0.5;]])],
          [gl_cv_func_expm1l_in_libm=yes],
          [gl_cv_func_expm1l_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_expm1l_in_libm = yes; then
      EXPM1L_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_expm1l_no_libm = yes \
     || test $gl_cv_func_expm1l_in_libm = yes; then
    HAVE_EXPM1L=1
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has expm1l() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECL([expm1l], , [HAVE_DECL_EXPM1L=0], [[#include <math.h>]])
    if test $REPLACE_EXPM1L = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether expm1l works],
        [gl_cv_func_expm1l_works],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $EXPM1L_LIBM"
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
#undef expm1l
extern
#ifdef __cplusplus
"C"
#endif
long double expm1l (long double);
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_expm1l) (long double) = argc ? expm1l : dummy;
  int result = 0;
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x, Mac OS X 10.5,
     NetBSD 8.0.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 11.358L;
    long double y = my_expm1l (x);
    long double z = my_expm1l (- x);
    volatile long double t = (1.0L + y) * z;
    long double err = (y + t) * TWO_LDBL_MANT_DIG;
    if (!(err >= -100.0L && err <= 100.0L))
      result |= 1;
  }
  return result;
}
            ]])],
            [gl_cv_func_expm1l_works=yes],
            [gl_cv_func_expm1l_works=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_expm1l_works="guessing yes" ;;
                              # Guess no on musl systems.
               *-musl*)       gl_cv_func_expm1l_works="guessing no" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_expm1l_works="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_expm1l_works="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_expm1l_works" in
        *yes) ;;
        *) REPLACE_EXPM1L=1 ;;
      esac
    fi
  else
    HAVE_EXPM1L=0
    HAVE_DECL_EXPM1L=0
  fi
  if test $HAVE_EXPM1L = 0 || test $REPLACE_EXPM1L = 1; then
    dnl Find libraries needed to link lib/expm1l.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_EXPM1])
      EXPM1L_LIBM="$EXPM1_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_EXPL])
      AC_REQUIRE([gl_FUNC_ROUNDL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      EXPM1L_LIBM=
      dnl Append $ISNANL_LIBM to EXPM1L_LIBM, avoiding gratuitous duplicates.
      case " $EXPM1L_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) EXPM1L_LIBM="$EXPM1L_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $EXPL_LIBM to EXPM1L_LIBM, avoiding gratuitous duplicates.
      case " $EXPM1L_LIBM " in
        *" $EXPL_LIBM "*) ;;
        *) EXPM1L_LIBM="$EXPM1L_LIBM $EXPL_LIBM" ;;
      esac
      dnl Append $ROUNDL_LIBM to EXPM1L_LIBM, avoiding gratuitous duplicates.
      case " $EXPM1L_LIBM " in
        *" $ROUNDL_LIBM "*) ;;
        *) EXPM1L_LIBM="$EXPM1L_LIBM $ROUNDL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to EXPM1L_LIBM, avoiding gratuitous duplicates.
      case " $EXPM1L_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) EXPM1L_LIBM="$EXPM1L_LIBM $LDEXPL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([EXPM1L_LIBM])
])
