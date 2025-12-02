# acosl.m4 serial 9
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ACOSL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare acosl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  ACOSL_LIBM=
  AC_CACHE_CHECK([whether acosl() can be used without linking with libm],
    [gl_cv_func_acosl_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             long double (* volatile funcptr) (long double) = acosl;
             long double x;]],
           [[return funcptr (x) > 1
                    || acosl (x) > 1;]])],
        [gl_cv_func_acosl_no_libm=yes],
        [gl_cv_func_acosl_no_libm=no])
    ])
  if test $gl_cv_func_acosl_no_libm = no; then
    AC_CACHE_CHECK([whether acosl() can be used with libm],
      [gl_cv_func_acosl_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               long double (* volatile funcptr) (long double) = acosl;
               long double x;]],
             [[return funcptr (x) > 1
                      || acosl (x) > 1;]])],
          [gl_cv_func_acosl_in_libm=yes],
          [gl_cv_func_acosl_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_acosl_in_libm = yes; then
      ACOSL_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_acosl_no_libm = yes \
     || test $gl_cv_func_acosl_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl Mac OS X 10.3 has acosl() in libc but doesn't declare it in <math.h>.
    AC_CHECK_DECL([acosl], , [HAVE_DECL_ACOSL=0], [[#include <math.h>]])
  else
    HAVE_DECL_ACOSL=0
    HAVE_ACOSL=0
    dnl Find libraries needed to link lib/acosl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_ACOS])
      ACOSL_LIBM="$ACOS_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ASINL])
      AC_REQUIRE([gl_FUNC_SQRTL])
      dnl Append $ASINL_LIBM to ACOSL_LIBM, avoiding gratuitous duplicates.
      case " $ACOSL_LIBM " in
        *" $ASINL_LIBM "*) ;;
        *) ACOSL_LIBM="$ACOSL_LIBM $ASINL_LIBM" ;;
      esac
      dnl Append $SQRTL_LIBM to ACOSL_LIBM, avoiding gratuitous duplicates.
      case " $ACOSL_LIBM " in
        *" $SQRTL_LIBM "*) ;;
        *) ACOSL_LIBM="$ACOSL_LIBM $SQRTL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([ACOSL_LIBM])
])
