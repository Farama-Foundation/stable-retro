# pty.m4 serial 14
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_PTY_LIB
# ----------
# Define automake variable PTY_LIB to the library needed (if any).
AC_DEFUN([gl_PTY_LIB],
[
  # Check for the library required for forkpty.
  PTY_LIB=
  save_LIBS="$LIBS"
  AC_SEARCH_LIBS([forkpty], [util],
    [if test "$ac_cv_search_forkpty" != "none required"; then
       PTY_LIB="$ac_cv_search_forkpty"
     fi])
  LIBS="$save_LIBS"
  AC_SUBST([PTY_LIB])
])

AC_DEFUN([gl_FUNC_FORKPTY],
[
  AC_REQUIRE([gl_PTY_LIB])
  AC_REQUIRE([gl_PTY_H])

  dnl We assume that forkpty exists (possibly in libc, possibly in libutil)
  dnl if and only if it is declared.
  AC_CHECK_DECLS([forkpty],,, [[
/* <sys/types.h> is a prerequisite of <libutil.h> on FreeBSD 8.0.  */
#include <sys/types.h>
#if HAVE_PTY_H
# include <pty.h>
#endif
#if HAVE_UTIL_H
# include <util.h>
#endif
#if HAVE_LIBUTIL_H
# include <libutil.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
]])
  if test $ac_cv_have_decl_forkpty = yes; then
    dnl The system has forkpty.
    dnl Prefer glibc's const-safe prototype, if available.
    AC_CACHE_CHECK([for const-safe forkpty signature],
      [gl_cv_func_forkpty_const],
      [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[
/* <sys/types.h> is a prerequisite of <libutil.h> on FreeBSD 8.0.  */
#include <sys/types.h>
#if HAVE_PTY_H
# include <pty.h>
#endif
#if HAVE_UTIL_H
# include <util.h>
#endif
#if HAVE_LIBUTIL_H
# include <libutil.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
          ]], [[
            int forkpty (int *, char *, struct termios const *,
                         struct winsize const *);
          ]])
        ],
        [gl_cv_func_forkpty_const=yes], [gl_cv_func_forkpty_const=no])
      ])
    if test $gl_cv_func_forkpty_const != yes; then
      REPLACE_FORKPTY=1
    fi
  else
    dnl The system does not have forkpty.
    HAVE_FORKPTY=0
  fi
  AC_DEFINE_UNQUOTED([HAVE_FORKPTY], [$HAVE_FORKPTY],
    [Define to 1 if the system has the 'forkpty' function.])
])

AC_DEFUN([gl_FUNC_OPENPTY],
[
  AC_REQUIRE([gl_PTY_LIB])
  AC_REQUIRE([gl_PTY_H])

  dnl Persuade Solaris <stdlib.h> to declare posix_openpt().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  dnl We assume that openpty exists (possibly in libc, possibly in libutil)
  dnl if and only if it is declared.
  AC_CHECK_DECLS([openpty],,, [[
/* <sys/types.h> is a prerequisite of <libutil.h> on FreeBSD 8.0.  */
#include <sys/types.h>
#if HAVE_PTY_H
# include <pty.h>
#endif
#if HAVE_UTIL_H
# include <util.h>
#endif
#if HAVE_LIBUTIL_H
# include <libutil.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
]])
  if test $ac_cv_have_decl_openpty = yes; then
    AC_DEFINE([HAVE_OPENPTY], [1],
      [Define to 1 if the system has the 'openpty' function.])
    dnl The system has openpty.
    dnl Prefer glibc's const-safe prototype, if available.
    AC_CACHE_CHECK([for const-safe openpty signature],
      [gl_cv_func_openpty_const],
      [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[
/* <sys/types.h> is a prerequisite of <libutil.h> on FreeBSD 8.0.  */
#include <sys/types.h>
#if HAVE_PTY_H
# include <pty.h>
#endif
#if HAVE_UTIL_H
# include <util.h>
#endif
#if HAVE_LIBUTIL_H
# include <libutil.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
          ]], [[
            int openpty (int *, int *, char *, struct termios const *,
                       struct winsize const *);
          ]])
        ],
        [gl_cv_func_openpty_const=yes], [gl_cv_func_openpty_const=no])
      ])
    if test $gl_cv_func_openpty_const != yes; then
      REPLACE_OPENPTY=1
    fi
  else
    dnl The system does not have openpty.
    HAVE_OPENPTY=0
    dnl Prerequisites of lib/openpty.c in this case.
    AC_CHECK_FUNCS([_getpty posix_openpt])
  fi
])

AC_DEFUN([gl_FUNC_LOGIN_TTY],
[
  AC_REQUIRE([gl_PTY_LIB])

  gl_saved_libs="$LIBS"
  LIBS="$LIBS $PTY_LIB"
  AC_CHECK_FUNCS([login_tty])
  LIBS="$gl_saved_LIBS"
])
