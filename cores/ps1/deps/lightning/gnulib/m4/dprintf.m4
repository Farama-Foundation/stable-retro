# dprintf.m4 serial 1
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_DPRINTF],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([dprintf])
  if test $ac_cv_func_dprintf = no; then
    HAVE_DPRINTF=0
    gl_REPLACE_DPRINTF
  fi
])

AC_DEFUN([gl_REPLACE_DPRINTF],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  AC_LIBOBJ([dprintf])
  if test $ac_cv_func_dprintf = yes; then
    REPLACE_DPRINTF=1
  fi
  gl_PREREQ_DPRINTF
])

# Prerequisites of lib/dprintf.c.
AC_DEFUN([gl_PREREQ_DPRINTF], [:])
