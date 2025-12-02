# sinhf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SINHF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_SINH])

  dnl Persuade glibc <math.h> to declare sinhf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether sinhf() exists. Assume that sinhf(), if it exists, is
  dnl defined in the same library as sinh().
  save_LIBS="$LIBS"
  LIBS="$LIBS $SINH_LIBM"
  AC_CHECK_FUNCS([sinhf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_sinhf = yes; then
    SINHF_LIBM="$SINH_LIBM"
  else
    HAVE_SINHF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_sinhf, not sinhf.
    AC_CHECK_DECLS([sinhf], [REPLACE_SINHF=1], , [[#include <math.h>]])
    SINHF_LIBM="$SINH_LIBM"
  fi
  AC_SUBST([SINHF_LIBM])
])
