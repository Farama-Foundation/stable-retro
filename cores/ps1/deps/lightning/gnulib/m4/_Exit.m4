# _Exit.m4 serial 2
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC__EXIT],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS([_Exit])
  if test $ac_cv_func__Exit = no; then
    HAVE__EXIT=0
  fi
])

# Prerequisites of lib/_Exit.c.
AC_DEFUN([gl_PREREQ__EXIT], [
  :
])
