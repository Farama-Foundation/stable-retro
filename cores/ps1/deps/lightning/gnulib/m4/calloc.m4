# calloc.m4 serial 23

# Copyright (C) 2004-2021 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

# Determine whether calloc (N, S) returns non-NULL when N*S is zero,
# and returns NULL when N*S overflows.
# If so, define HAVE_CALLOC.  Otherwise, define calloc to rpl_calloc
# and arrange to use a calloc wrapper function that does work in that case.

# _AC_FUNC_CALLOC_IF([IF-WORKS], [IF-NOT])
# -------------------------------------
# If 'calloc (0, 0)' is properly handled, run IF-WORKS, otherwise, IF-NOT.
AC_DEFUN([_AC_FUNC_CALLOC_IF],
[
  AC_REQUIRE([AC_TYPE_SIZE_T])dnl
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([for GNU libc compatible calloc],
    [ac_cv_func_calloc_0_nonnull],
    [if test $cross_compiling != yes; then
       ac_cv_func_calloc_0_nonnull=yes
       AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [AC_INCLUDES_DEFAULT],
            [[int result = 0;
              char * volatile p = calloc (0, 0);
              if (!p)
                result |= 1;
              free (p);
              return result;
            ]])],
         [],
         [ac_cv_func_calloc_0_nonnull=no])
       AC_RUN_IFELSE(
         [AC_LANG_PROGRAM(
            [AC_INCLUDES_DEFAULT],
            [[int result;
              typedef struct { char c[8]; } S8;
              size_t n = (size_t) -1 / sizeof (S8) + 2;
              S8 * volatile s = calloc (n, sizeof (S8));
              if (s)
                {
                  s[0].c[0] = 1;
                  if (s[n - 1].c[0])
                    result = 0;
                  else
                    result = 2;
                }
              else
                result = 3;
              free (s);
              return result;
            ]])],
         dnl The exit code of this program is 0 if calloc() succeeded with a
         dnl wrap-around bug (which it shouldn't), 2 if calloc() succeeded in
         dnl a non-flat address space, 3 if calloc() failed, or 1 if some leak
         dnl sanitizer terminated the program as a result of the calloc() call.
         [ac_cv_func_calloc_0_nonnull=no],
         [])
     else
       case "$host_os" in
                        # Guess yes on glibc systems.
         *-gnu* | gnu*) ac_cv_func_calloc_0_nonnull="guessing yes" ;;
                        # Guess yes on musl systems.
         *-musl*)       ac_cv_func_calloc_0_nonnull="guessing yes" ;;
                        # Guess yes on native Windows.
         mingw*)        ac_cv_func_calloc_0_nonnull="guessing yes" ;;
                        # If we don't know, obey --enable-cross-guesses.
         *)             ac_cv_func_calloc_0_nonnull="$gl_cross_guess_normal" ;;
       esac
     fi
    ])
  case "$ac_cv_func_calloc_0_nonnull" in
    *yes)
      $1
      ;;
    *)
      $2
      ;;
  esac
])# AC_FUNC_CALLOC


# gl_FUNC_CALLOC_GNU
# ------------------
# Report whether 'calloc (0, 0)' is properly handled, and replace calloc if
# needed.
AC_DEFUN([gl_FUNC_CALLOC_GNU],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  _AC_FUNC_CALLOC_IF(
    [AC_DEFINE([HAVE_CALLOC_GNU], [1],
               [Define to 1 if your system has a GNU libc compatible 'calloc'
                function, and to 0 otherwise.])],
    [AC_DEFINE([HAVE_CALLOC_GNU], [0])
     REPLACE_CALLOC=1
    ])
])# gl_FUNC_CALLOC_GNU


# gl_FUNC_CALLOC_POSIX
# --------------------
# Test whether 'calloc' is POSIX compliant (sets errno to ENOMEM when it
# fails), and replace calloc if it is not.
AC_DEFUN([gl_FUNC_CALLOC_POSIX],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([gl_CHECK_MALLOC_POSIX])
  if test $gl_cv_func_malloc_posix = yes; then
    AC_DEFINE([HAVE_CALLOC_POSIX], [1],
      [Define if the 'calloc' function is POSIX compliant.])
  else
    REPLACE_CALLOC=1
  fi
])
