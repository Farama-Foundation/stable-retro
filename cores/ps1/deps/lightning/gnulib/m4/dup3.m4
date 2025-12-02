# dup3.m4 serial 5
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_DUP3],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  dnl Persuade glibc <unistd.h> to declare dup3().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([dup3 setdtablesize])
  if test $ac_cv_func_dup3 != yes; then
    HAVE_DUP3=0
  fi

dnl Not needed yet, because dup3 is unconditionally replaced.
dnl  dnl Replace dup3() for supporting the gnulib-defined fchdir() function,
dnl  dnl to keep fchdir's bookkeeping up-to-date.
dnl  m4_ifdef([gl_FUNC_FCHDIR], [
dnl    gl_TEST_FCHDIR
dnl    if test $HAVE_FCHDIR = 0; then
dnl      if test $HAVE_DUP3 = 1; then
dnl        REPLACE_DUP3=1
dnl      fi
dnl    fi
dnl  ])
])
