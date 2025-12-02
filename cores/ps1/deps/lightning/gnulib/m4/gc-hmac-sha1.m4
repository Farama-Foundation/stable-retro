# gc-hmac-sha1.m4 serial 3
dnl Copyright (C) 2005, 2007, 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GC_HMAC_SHA1],
[
  AC_REQUIRE([gl_GC])
  if test "$ac_cv_libgcrypt" != yes; then
    gl_SHA1
    gl_MEMXOR
  fi
])
