# gc-sm3.m4 serial 2
dnl Copyright (C) 2017-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GC_SM3],
[
  AC_REQUIRE([gl_GC])
  AC_CACHE_CHECK([whether libgcrypt supports SM3],
    [gl_cv_libcrypt_md_sm3],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
          #include <gcrypt.h>
          int a = GCRY_MD_SM3;
          ]], [[]])
       ],
       [gl_cv_libcrypt_md_sm3=yes],
       [gl_cv_libcrypt_md_sm3=no])
    ])
  if test $gl_cv_libcrypt_md_sm3 = yes; then
    AC_DEFINE([LIBGCRYPT_HAS_MD_SM3], [1],
      [Define if libgcrypt supports the MD algorithm SM3.])
  fi
])
