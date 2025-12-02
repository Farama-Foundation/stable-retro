# gc-camellia.m4 serial 3
dnl Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GC_CAMELLIA],
[
  AC_REQUIRE([gl_GC])
  if test "$ac_cv_libgcrypt" = yes; then
    AC_CACHE_CHECK([for camellia in libgcrypt], [gl_cv_libgcrypt_camellia], [
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gcrypt.h>]],
          [[return gcry_cipher_open (NULL, GCRY_CIPHER_CAMELLIA128, 0, 0);]])],
        [gl_cv_libgcrypt_camellia=yes],
        [gl_cv_libgcrypt_camellia=no])])
    if test "$gl_cv_libgcrypt_camellia" = yes; then
      AC_DEFINE([HAVE_CAMELLIA], [1], [Define if your libgcrypt supports Camellia.])
    fi
  fi
])
