# year2038.m4 serial 3
dnl Copyright (C) 2017-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Attempt to ensure that 'time_t' is a 64-bit type
dnl and that the functions time(), stat(), etc. return 64-bit times.

AC_DEFUN([gl_YEAR2038_EARLY],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  case "$host_os" in
    mingw*)
      AC_DEFINE([__MINGW_USE_VC2005_COMPAT], [1],
        [For 64-bit time_t on 32-bit mingw.])
      ;;
  esac
])

AC_DEFUN([gl_YEAR2038],
[
  dnl On many systems, time_t is already a 64-bit type.
  dnl On those systems where time_t is still 32-bit, it requires kernel
  dnl and libc support to make it 64-bit. For glibc on Linux/x86, this
  dnl is work in progress; see
  dnl <https://sourceware.org/glibc/wiki/Y2038ProofnessDesign>.
  dnl
  dnl On native Windows, the system include files define types __time32_t
  dnl and __time64_t. By default, time_t is an alias of
  dnl   - __time32_t on 32-bit mingw,
  dnl   - __time64_t on 64-bit mingw and on MSVC (since MSVC 8).
  dnl But when compiling with -D__MINGW_USE_VC2005_COMPAT, time_t is an
  dnl alias of __time64_t.
  dnl And when compiling with -D_USE_32BIT_TIME_T, time_t is an alias of
  dnl __time32_t.
  AC_CACHE_CHECK([for 64-bit time_t], [gl_cv_type_time_t_64],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <time.h>
            int verify_time_t_size[sizeof (time_t) >= 8 ? 1 : -1];
          ]],
          [[]])],
       [gl_cv_type_time_t_64=yes], [gl_cv_type_time_t_64=no])
    ])
  if test $gl_cv_type_time_t_64 = no; then
    AC_COMPILE_IFELSE(
      [AC_LANG_SOURCE(
         [[#ifdef _USE_32BIT_TIME_T
             int ok;
           #else
             error fail
           #endif
         ]])],
      [AC_MSG_FAILURE([This package requires a 64-bit 'time_t' type. Remove _USE_32BIT_TIME_T from the compiler flags.])],
      [# If TIME_T_32_BIT_OK is "no" (the default) and not cross-compiling
       # and 'touch' works with a large timestamp, then evidently 64-bit time_t
       # is desired and supported, so fail and ask the builder to fix the
       # problem.  Otherwise, just warn the builder.
       if test "${TIME_T_32_BIT_OK-no}" = no \
          && test $cross_compiling = no \
          && TZ=UTC0 touch -t 210602070628.16 conftest.time 2>/dev/null; then
         rm -f conftest.time
         AC_MSG_FAILURE([This package requires a 64-bit 'time_t' type, which your system appears to support. You might try configuring with 'CPPFLAGS="-m64" LDFLAGS="-m64"'. To build with a 32-bit time_t anyway (not recommended), configure with 'TIME_T_32_BIT_OK=yes'.])
       else
         AC_MSG_WARN([This package requires a 64-bit 'time_t' type if there is any way to access timestamps outside the year range 1901-2038 on your platform. Perhaps you should configure with 'CPPFLAGS="-m64" LDFLAGS="-m64"'?])
       fi
      ])
  fi
])
