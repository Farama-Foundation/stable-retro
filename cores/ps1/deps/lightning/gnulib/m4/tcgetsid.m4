# tcgetsid.m4 serial 5
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_TCGETSID],
[
  AC_REQUIRE([gl_TERMIOS_H_DEFAULTS])

  dnl Persuade glibc <termios.h> to declare tcgetsid().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_DECLS([tcgetsid], , , [[#include <termios.h>]])
  if test $ac_cv_have_decl_tcgetsid = no; then
    HAVE_DECL_TCGETSID=0
  fi

  dnl We can't use AC_CHECK_FUNC here, because tcgetsid() is defined as a
  dnl static inline function when compiling for Android 4.4 or older.
  AC_CACHE_CHECK([for tcgetsid], [gl_cv_func_tcgetsid],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <termios.h>]],
          [[return tcgetsid(0);]])
       ],
       [gl_cv_func_tcgetsid=yes],
       [gl_cv_func_tcgetsid=no])
    ])
  if test $gl_cv_func_tcgetsid = yes; then
    HAVE_TCGETSID=1
  else
    HAVE_TCGETSID=0
  fi
])

# Prerequisites of lib/tcgetsid.c.
AC_DEFUN([gl_PREREQ_TCGETSID], [
  :
])
