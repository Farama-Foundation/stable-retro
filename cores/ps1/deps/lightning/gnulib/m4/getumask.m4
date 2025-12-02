# getumask.m4 serial 1
dnl Copyright 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_GETUMASK],
[
  AC_REQUIRE([gl_SYS_STAT_H_DEFAULTS])

  dnl Persuade glibc <sys/stat.h> to declare getumask().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([getumask])
  if test $ac_cv_func_getumask = no; then
    HAVE_GETUMASK=0
  fi
])

# Prerequisites of lib/getumask.c.
AC_DEFUN([gl_PREREQ_GETUMASK],
[
  :
])
