# powf.m4 serial 2
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_POWF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_POW])

  dnl Persuade glibc <math.h> to declare powf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether powf() exists. Assume that powf(), if it exists, is
  dnl defined in the same library as pow().
  save_LIBS="$LIBS"
  LIBS="$LIBS $POW_LIBM"
  AC_CHECK_FUNCS([powf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_powf = yes; then
    POWF_LIBM="$POW_LIBM"
  else
    HAVE_POWF=0
    POWF_LIBM="$POW_LIBM"
  fi
  AC_SUBST([POWF_LIBM])
])
