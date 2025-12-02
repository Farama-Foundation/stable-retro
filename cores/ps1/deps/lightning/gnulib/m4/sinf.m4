# sinf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SINF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_SIN])

  dnl Persuade glibc <math.h> to declare sinf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether sinf() exists. Assume that sinf(), if it exists, is
  dnl defined in the same library as sin().
  save_LIBS="$LIBS"
  LIBS="$LIBS $SIN_LIBM"
  AC_CHECK_FUNCS([sinf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_sinf = yes; then
    SINF_LIBM="$SIN_LIBM"
  else
    HAVE_SINF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_sinf, not sinf.
    AC_CHECK_DECLS([sinf], [REPLACE_SINF=1], , [[#include <math.h>]])
    SINF_LIBM="$SIN_LIBM"
  fi
  AC_SUBST([SINF_LIBM])
])
