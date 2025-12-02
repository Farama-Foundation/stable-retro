# cosf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_COSF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_COS])

  dnl Persuade glibc <math.h> to declare cosf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether cosf() exists. Assume that cosf(), if it exists, is
  dnl defined in the same library as cos().
  save_LIBS="$LIBS"
  LIBS="$LIBS $COS_LIBM"
  AC_CHECK_FUNCS([cosf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_cosf = yes; then
    COSF_LIBM="$COS_LIBM"
  else
    HAVE_COSF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_cosf, not cosf.
    AC_CHECK_DECLS([cosf], [REPLACE_COSF=1], , [[#include <math.h>]])
    COSF_LIBM="$COS_LIBM"
  fi
  AC_SUBST([COSF_LIBM])
])
