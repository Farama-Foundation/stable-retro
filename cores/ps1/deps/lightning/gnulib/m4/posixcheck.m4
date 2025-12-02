# posixcheck.m4 serial 1
dnl Copyright (C) 2018-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_POSIXCHECK],
[
  AH_VERBATIM([POSIXCHECK],
    [/* Define to enable warnings for determining which Gnulib modules to use, for
   portability of POSIX functions.  This does not work well in C++ mode.  */
#ifndef __cplusplus
# define GNULIB_POSIXCHECK 1
#endif
])
])
