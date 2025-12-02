# random.m4 serial 4
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RANDOM],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])

  dnl We can't use AC_CHECK_FUNC here, because random() is defined as a
  dnl static inline function when compiling for Android 4.4 or older.
  AC_CACHE_CHECK([for random], [gl_cv_func_random],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <stdlib.h>]],
          [[return random() == 0;]])
       ],
       [gl_cv_func_random=yes],
       [gl_cv_func_random=no])
    ])
  if test $gl_cv_func_random = no; then
    HAVE_RANDOM=0
    HAVE_INITSTATE=0
    HAVE_SETSTATE=0
  else
    AC_CHECK_FUNCS([initstate setstate])
    if test $ac_cv_func_initstate = no; then
      HAVE_INITSTATE=0
    fi
    if test $ac_cv_func_setstate = no; then
      HAVE_SETSTATE=0
    fi
    if test $ac_cv_func_initstate = no || test $ac_cv_func_setstate = no; then
      dnl In order to define initstate or setstate, we need to define all the
      dnl functions at once.
      REPLACE_RANDOM=1
      if test $ac_cv_func_initstate = yes; then
        REPLACE_INITSTATE=1
      fi
      if test $ac_cv_func_setstate = yes; then
        REPLACE_SETSTATE=1
      fi
    fi
  fi

  AC_CHECK_DECLS_ONCE([initstate])
  if test $ac_cv_have_decl_initstate = no; then
    HAVE_DECL_INITSTATE=0
  fi

  AC_CHECK_DECLS_ONCE([setstate])
  if test $ac_cv_have_decl_setstate = no; then
    HAVE_DECL_SETSTATE=0
  fi
])

# Prerequisites of lib/random.c.
AC_DEFUN([gl_PREREQ_RANDOM], [
  :
])
