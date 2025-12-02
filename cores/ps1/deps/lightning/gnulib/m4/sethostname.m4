# sethostname.m4 serial 1
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Ensure
# - the sethostname() function,
# - the HOST_NAME_MAX macro in <limits.h>.
AC_DEFUN([gl_FUNC_SETHOSTNAME],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  gl_PREREQ_HOST_NAME_MAX

  AC_CHECK_FUNCS([sethostname])
  if test $ac_cv_func_sethostname = no; then
    HAVE_SETHOSTNAME=0
  fi

  AC_CHECK_DECLS([sethostname])
  if test $ac_cv_have_decl_sethostname = no; then
    HAVE_DECL_SETHOSTNAME=0
  fi
])
