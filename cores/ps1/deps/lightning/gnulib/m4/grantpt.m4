# grantpt.m4 serial 3
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_GRANTPT],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  dnl Persuade glibc <stdlib.h> to declare grantpt().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  dnl We can't use AC_CHECK_FUNC here, because grantpt() is defined as a
  dnl static inline function when compiling for Android 4.4 or older.
  AC_CACHE_CHECK([for grantpt], [gl_cv_func_grantpt],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <stdlib.h>]],
          [[return grantpt(0);]])
       ],
       [gl_cv_func_grantpt=yes],
       [gl_cv_func_grantpt=no])
    ])
  if test $gl_cv_func_grantpt = no; then
    HAVE_GRANTPT=0
  fi
])

# Prerequisites of lib/grantpt.c.
AC_DEFUN([gl_PREREQ_GRANTPT], [
  AC_CHECK_FUNCS([setrlimit])
])
