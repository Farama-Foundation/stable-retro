# serial 14

# Copyright (C) 2000-2001, 2005-2006, 2009-2021 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# When rmdir fails because the specified directory is not empty, it sets
# errno to some value, usually ENOTEMPTY.  However, on some AIX systems,
# ENOTEMPTY is mistakenly defined to be EEXIST.  To work around this, and
# in general, to avoid depending on the use of any particular symbol, this
# test runs a test to determine the actual numeric value.
AC_DEFUN([gl_FUNC_RMDIR_NOTEMPTY],
[dnl
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CHECK_HEADERS_ONCE([unistd.h])
  AC_CACHE_CHECK([for rmdir-not-empty errno value],
    [gl_cv_func_rmdir_errno_not_empty],
    [
      # Arrange for deletion of the temporary directory this test creates.
      ac_clean_files="$ac_clean_files confdir2"
      mkdir confdir2; : > confdir2/file
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
]GL_MDA_DEFINES[
        int main ()
        {
          FILE *s;
          int val;
          rmdir ("confdir2");
          val = errno;
          s = fopen ("confdir2/errno", "w");
          fprintf (s, "%d\n", val);
          fclose (s);
          return 0;
        }
        ]])],
        [gl_cv_func_rmdir_errno_not_empty=`cat confdir2/errno`],
        [gl_cv_func_rmdir_errno_not_empty='configure error in rmdir-errno.m4'],
        [case "$host_os" in
                   # Guess ENOTEMPTY = 41 on native Windows.
           mingw*) gl_cv_func_rmdir_errno_not_empty=ENOTEMPTY ;;
           *)      gl_cv_func_rmdir_errno_not_empty=ENOTEMPTY ;;
         esac
        ])
    ]
  )

  AC_DEFINE_UNQUOTED([RMDIR_ERRNO_NOT_EMPTY],
    [$gl_cv_func_rmdir_errno_not_empty],
    [the value to which errno is set when rmdir fails on a nonempty directory])
])
