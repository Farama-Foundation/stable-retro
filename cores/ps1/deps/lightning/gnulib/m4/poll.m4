# poll.m4 serial 20
dnl Copyright (c) 2003, 2005-2007, 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_POLL],
[
  AC_REQUIRE([gl_POLL_H])
  AC_REQUIRE([gl_SOCKETS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  if test $ac_cv_header_poll_h = no; then
    ac_cv_func_poll=no
    gl_cv_func_poll=no
  else
    AC_CHECK_FUNC([poll],
      [# Check whether poll() works on special files (like /dev/null) and
       # and ttys (like /dev/tty). On Mac OS X 10.4.0 and AIX 5.3, it doesn't.
       AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <fcntl.h>
#include <poll.h>
]GL_MDA_DEFINES[
         int main()
         {
           int result = 0;
           struct pollfd ufd;
           /* Try /dev/null for reading.  */
           ufd.fd = open ("/dev/null", O_RDONLY);
           /* If /dev/null does not exist, it's not Mac OS X nor AIX. */
           if (ufd.fd >= 0)
             {
               ufd.events = POLLIN;
               ufd.revents = 0;
               if (!(poll (&ufd, 1, 0) == 1 && ufd.revents == POLLIN))
                 result |= 1;
             }
           /* Try /dev/null for writing.  */
           ufd.fd = open ("/dev/null", O_WRONLY);
           /* If /dev/null does not exist, it's not Mac OS X nor AIX. */
           if (ufd.fd >= 0)
             {
               ufd.events = POLLOUT;
               ufd.revents = 0;
               if (!(poll (&ufd, 1, 0) == 1 && ufd.revents == POLLOUT))
                 result |= 2;
             }
           /* Trying /dev/tty may be too environment dependent.  */
           return result;
         }]])],
         [gl_cv_func_poll=yes],
         [gl_cv_func_poll=no],
         [# When cross-compiling, assume that poll() works everywhere except on
          # Mac OS X or AIX, regardless of its version.
          AC_EGREP_CPP([MacOSX], [
#if (defined(__APPLE__) && defined(__MACH__)) || defined(_AIX)
This is MacOSX or AIX
#endif
], [gl_cv_func_poll="guessing no"], [gl_cv_func_poll="guessing yes"])])])
  fi
  case "$gl_cv_func_poll" in
    *yes) ;;
    *)
      AC_CHECK_FUNC([poll], [ac_cv_func_poll=yes], [ac_cv_func_poll=no])
      if test $ac_cv_func_poll = no; then
        HAVE_POLL=0
      else
        REPLACE_POLL=1
      fi
      ;;
  esac
  if test $HAVE_POLL = 0 || test $REPLACE_POLL = 1; then
    :
  else
    AC_DEFINE([HAVE_POLL], [1],
      [Define to 1 if you have the 'poll' function and it works.])
  fi

  dnl Determine the needed libraries.
  LIB_POLL="$LIBSOCKET"
  if test $HAVE_POLL = 0 || test $REPLACE_POLL = 1; then
    case "$host_os" in
      mingw*)
        dnl On the MSVC platform, the function MsgWaitForMultipleObjects
        dnl (used in lib/poll.c) requires linking with -luser32. On mingw,
        dnl it is implicit.
        AC_LINK_IFELSE(
          [AC_LANG_SOURCE([[
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int
main ()
{
  MsgWaitForMultipleObjects (0, NULL, 0, 0, 0);
  return 0;
}]])],
          [],
          [LIB_POLL="$LIB_POLL -luser32"])
        ;;
    esac
  fi
  AC_SUBST([LIB_POLL])
])

# Prerequisites of lib/poll.c.
AC_DEFUN([gl_PREREQ_POLL],
[
  AC_CHECK_HEADERS_ONCE([sys/ioctl.h sys/filio.h])
])
