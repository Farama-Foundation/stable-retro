# fegetround.m4 serial 1
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FEGETROUND],
[
  dnl Determine FEGETROUND_LIBM.
  gl_MATHFUNC([fegetround], [int], [(void)], [#include <fenv.h>])
  if test $gl_cv_func_fegetround_no_libm = no \
     && test $gl_cv_func_fegetround_in_libm = no; then
    HAVE_FEGETROUND=0
  else
    HAVE_FEGETROUND=1
    AC_DEFINE([HAVE_FEGETROUND], [1],
      [Define to 1 if you have the 'fegetround' function.])
  fi
  AC_SUBST([FEGETROUND_LIBM])
])
