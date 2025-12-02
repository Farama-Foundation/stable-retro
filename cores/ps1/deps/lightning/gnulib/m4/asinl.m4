# asinl.m4 serial 9
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ASINL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare asinl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  ASINL_LIBM=
  AC_CACHE_CHECK([whether asinl() can be used without linking with libm],
    [gl_cv_func_asinl_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             long double (* volatile funcptr) (long double) = asinl;
             long double x;]],
           [[return funcptr (x) > 1
                    || asinl (x) > 1;]])],
        [gl_cv_func_asinl_no_libm=yes],
        [gl_cv_func_asinl_no_libm=no])
    ])
  if test $gl_cv_func_asinl_no_libm = no; then
    AC_CACHE_CHECK([whether asinl() can be used with libm],
      [gl_cv_func_asinl_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               long double (* volatile funcptr) (long double) = asinl;
               long double x;]],
             [[return funcptr (x) > 1
                      || asinl (x) > 1;]])],
          [gl_cv_func_asinl_in_libm=yes],
          [gl_cv_func_asinl_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_asinl_in_libm = yes; then
      ASINL_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_asinl_no_libm = yes \
     || test $gl_cv_func_asinl_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl Mac OS X 10.3 has asinl() in libc but doesn't declare it in <math.h>.
    AC_CHECK_DECL([asinl], , [HAVE_DECL_ASINL=0], [[#include <math.h>]])
  else
    HAVE_DECL_ASINL=0
    HAVE_ASINL=0
    dnl Find libraries needed to link lib/asinl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_ASIN])
      ASINL_LIBM="$ASIN_LIBM"
    else
      AC_REQUIRE([gl_FUNC_SQRTL])
      dnl Append $SQRTL_LIBM to ASINL_LIBM, avoiding gratuitous duplicates.
      case " $ASINL_LIBM " in
        *" $SQRTL_LIBM "*) ;;
        *) ASINL_LIBM="$ASINL_LIBM $SQRTL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([ASINL_LIBM])
])
