# unlockpt.m4 serial 2
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_UNLOCKPT],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  dnl Persuade glibc <stdlib.h> to declare unlockpt().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS([unlockpt])
  if test $ac_cv_func_unlockpt = no; then
    HAVE_UNLOCKPT=0
  fi
])

# Prerequisites of lib/unlockpt.c.
AC_DEFUN([gl_PREREQ_UNLOCKPT], [
  AC_CHECK_FUNCS([revoke])
])
