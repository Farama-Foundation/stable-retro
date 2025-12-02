# valgrind-tests.m4 serial 6
dnl Copyright (C) 2008-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson

# gl_VALGRIND_TESTS()
# -------------------
# Check if valgrind is available, and set VALGRIND to it if available.
AC_DEFUN([gl_VALGRIND_TESTS],
[
  AC_ARG_ENABLE([valgrind-tests],
    AS_HELP_STRING([--disable-valgrind-tests],
                   [don't try to run self tests under valgrind]),
    [opt_valgrind_tests=$enableval], [opt_valgrind_tests=yes])

  # Run self-tests under valgrind?
  if test "$opt_valgrind_tests" = "yes" && test "$cross_compiling" = no; then
    AC_CHECK_PROGS([VALGRIND], [valgrind])

    if test -n "$VALGRIND"; then
      dnl On Ubuntu 16.04, /usr/bin/valgrind works only on 64-bit executables
      dnl but fails on 32-bit executables (with exit code 1) and on x86_64-x32
      dnl executables (with exit code 126).
      AC_CACHE_CHECK([whether valgrind works on executables produced by the compiler],
        [gl_cv_prog_valgrind_works],
        [AC_RUN_IFELSE(
           [AC_LANG_SOURCE([[int main () { return 0; }]])],
           [$VALGRIND ./conftest$ac_exeext 2>/dev/null
            if test $? = 0; then
              gl_cv_prog_valgrind_works=yes
            else
              gl_cv_prog_valgrind_works=no
            fi
           ],
           [gl_cv_prog_valgrind_works=no])
        ])
      if test $gl_cv_prog_valgrind_works != yes; then
        VALGRIND=
      fi
    fi

    if test -n "$VALGRIND"; then
      AC_CACHE_CHECK([for valgrind options for tests],
        [gl_cv_opt_valgrind_tests],
        [gl_valgrind_opts='-q --error-exitcode=1 --leak-check=full'
         if $VALGRIND $gl_valgrind_opts ls > /dev/null 2>&1; then
           gl_cv_opt_valgrind_tests="$gl_valgrind_opts"
         else
           gl_cv_opt_valgrind_tests=no
         fi
        ])
      if test "$gl_cv_opt_valgrind_tests" != no; then
        VALGRIND="$VALGRIND $gl_cv_opt_valgrind_tests"
      fi
    fi
  fi
])
