# ttyname_r.m4 serial 11
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_TTYNAME_R],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  dnl Persuade Solaris <unistd.h> to provide the POSIX compliant declaration of
  dnl ttyname_r().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_DECLS_ONCE([ttyname_r])
  if test $ac_cv_have_decl_ttyname_r = no; then
    HAVE_DECL_TTYNAME_R=0
  fi

  AC_CHECK_FUNCS([ttyname_r])
  if test $ac_cv_func_ttyname_r = no; then
    HAVE_TTYNAME_R=0
  else
    HAVE_TTYNAME_R=1
    dnl On Mac OS X 10.4 (and Solaris 10 without gl_USE_SYSTEM_EXTENSIONS)
    dnl the return type is 'char *', not 'int'.
    AC_CACHE_CHECK([whether ttyname_r is compatible with its POSIX signature],
      [gl_cv_func_ttyname_r_posix],
      [AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <stddef.h>
              #include <unistd.h>]],
            [[*ttyname_r (0, NULL, 0);]])
         ],
         [gl_cv_func_ttyname_r_posix=no],
         [gl_cv_func_ttyname_r_posix=yes])
      ])
    if test $gl_cv_func_ttyname_r_posix = no; then
      REPLACE_TTYNAME_R=1
    else
      AC_DEFINE([HAVE_POSIXDECL_TTYNAME_R], [1],
        [Define if the ttyname_r function has a POSIX compliant declaration.])
      dnl On Solaris 10, both ttyname_r functions (the one with the non-POSIX
      dnl declaration and the one with the POSIX declaration) refuse to do
      dnl anything when the output buffer is less than 128 bytes large.
      dnl On OSF/1 5.1, ttyname_r ignores the buffer size and assumes the
      dnl buffer is large enough.
      dnl On Android 4.3, ttyname_r is a stub that prints
      dnl "int ttyname_r(int, char*, size_t)(3) is not implemented on Android"
      dnl on stderr and returns -ERANGE.
      AC_REQUIRE([AC_CANONICAL_HOST])
      case "$host_os" in
        linux*-android*)
          AC_CACHE_CHECK([whether ttyname_r works at least minimally],
            [gl_cv_func_ttyname_r_not_stub],
            [AC_RUN_IFELSE(
               [AC_LANG_SOURCE([[
#include <errno.h>
#include <unistd.h>
]GL_MDA_DEFINES[
int
main (void)
{
  char buf[80];
  close(2);
  return ttyname_r (-1, buf, sizeof (buf)) == -ERANGE;
}]])],
               [gl_cv_func_ttyname_r_not_stub=yes],
               [gl_cv_func_ttyname_r_not_stub=no],
               [# Guess no on Android.
                gl_cv_func_ttyname_r_not_stub="guessing no"
               ])
            ])
          case "$gl_cv_func_ttyname_r_not_stub" in
            *yes) ;;
            *) REPLACE_TTYNAME_R=1 ;;
          esac
          ;;
      esac
      if test $REPLACE_TTYNAME_R = 0; then
        AC_CACHE_CHECK([whether ttyname_r works with small buffers],
          [gl_cv_func_ttyname_r_works],
          [
            dnl Initial guess, used when cross-compiling or when /dev/tty cannot
            dnl be opened.
changequote(,)dnl
            case "$host_os" in
                        # Guess no on Solaris.
              solaris*) gl_cv_func_ttyname_r_works="guessing no" ;;
                        # Guess no on OSF/1.
              osf*)     gl_cv_func_ttyname_r_works="guessing no" ;;
                        # Guess yes otherwise.
              *)        gl_cv_func_ttyname_r_works="guessing yes" ;;
            esac
changequote([,])dnl
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#include <fcntl.h>
#include <unistd.h>
]GL_MDA_DEFINES[
int
main (void)
{
  int result = 0;
  int fd;
  char buf[31]; /* use any size < 128 here */

  fd = open ("/dev/tty", O_RDONLY);
  if (fd < 0)
    result |= 16;
  else if (ttyname_r (fd, buf, sizeof (buf)) != 0)
    result |= 17;
  else if (ttyname_r (fd, buf, 1) == 0)
    result |= 18;
  return result;
}]])],
              [gl_cv_func_ttyname_r_works=yes],
              [case $? in
                 17 | 18) gl_cv_func_ttyname_r_works=no ;;
               esac],
              [:])
          ])
        case "$gl_cv_func_ttyname_r_works" in
          *yes) ;;
          *) REPLACE_TTYNAME_R=1 ;;
        esac
      fi
    fi
  fi
])

# Prerequisites of lib/ttyname_r.c.
AC_DEFUN([gl_PREREQ_TTYNAME_R], [
  AC_CHECK_FUNCS([ttyname])
])
