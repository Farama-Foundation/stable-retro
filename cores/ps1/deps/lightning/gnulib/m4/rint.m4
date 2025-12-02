# rint.m4 serial 4
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RINT],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Determine RINT_LIBM.
  dnl The reason for the extra code is:
  dnl In AIX 7.1 with gcc 4.2, when optimization is turned on, calls to
  dnl rint() with simple arguments are turned into rintf() calls by the
  dnl compiler. But while rint() is resides in libc, rintf() is in libm.
  gl_MATHFUNC([rint], [double], [(double)], [], [y += rint (2.0);])
  if test $gl_cv_func_rint_no_libm = no \
     && test $gl_cv_func_rint_in_libm = no; then
    HAVE_RINT=0
    RINT_LIBM=
  fi
  AC_SUBST([RINT_LIBM])
])
