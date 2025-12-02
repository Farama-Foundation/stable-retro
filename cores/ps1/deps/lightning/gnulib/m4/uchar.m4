# uchar.m4 serial 16
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.
dnl Prepare the overridden <uchar.h>.

AC_DEFUN_ONCE([gl_UCHAR_H],
[
  AC_REQUIRE([gl_UCHAR_H_DEFAULTS])

  gl_CHECK_NEXT_HEADERS([uchar.h])
  if test $ac_cv_header_uchar_h = yes; then
    HAVE_UCHAR_H=1
  else
    HAVE_UCHAR_H=0
  fi
  AC_SUBST([HAVE_UCHAR_H])

  gl_TYPE_CHAR16_T
  gl_TYPE_CHAR32_T

  dnl In C++ mode, clang defines 'char16_t' and 'char32_t' as built-in types
  dnl on some platforms (e.g. OpenBSD 6.7), and as types defined by many
  dnl header files (<limits.h>, <stddef.h>, <stdint.h>, <stdio.h>, <stdlib.h>
  dnl and others) on some platforms (e.g. Mac OS X 10.13).
  m4_ifdef([gl_ANSI_CXX], [AC_REQUIRE([gl_ANSI_CXX])])
  CXX_HAS_UCHAR_TYPES=0
  if test $HAVE_UCHAR_H = 0; then
    if test "$CXX" != no; then
      AC_CACHE_CHECK([whether the C++ compiler predefines the <uchar.h> types],
        [gl_cv_cxx_has_uchar_types],
        [dnl We can't use AC_LANG_PUSH([C++]) and AC_LANG_POP([C++]) here, due to
         dnl an autoconf bug <https://savannah.gnu.org/support/?110294>.
         cat > conftest.cpp <<\EOF
#include <stddef.h>
char16_t a;
char32_t b;
EOF
         gl_command="$CXX $CXXFLAGS $CPPFLAGS -c conftest.cpp"
         if AC_TRY_EVAL([gl_command]); then
           gl_cv_cxx_has_uchar_types=yes
         else
           gl_cv_cxx_has_uchar_types=no
         fi
         rm -fr conftest*
        ])
      if test $gl_cv_cxx_has_uchar_types = yes; then
        CXX_HAS_UCHAR_TYPES=1
      fi
    fi
  fi
  AC_SUBST([CXX_HAS_UCHAR_TYPES])

  dnl Test whether a 'char32_t' can hold more characters than a 'wchar_t'.
  gl_STDINT_BITSIZEOF([wchar_t], [gl_STDINT_INCLUDES])
  if test $BITSIZEOF_WCHAR_T -lt 32; then
    SMALL_WCHAR_T=1
  else
    SMALL_WCHAR_T=0
  fi
  dnl SMALL_WCHAR_T is expected to be 1 on 32-bit AIX, Cygwin, native Windows.
  AC_SUBST([SMALL_WCHAR_T])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use, and which is not
  dnl guaranteed by C11.
  gl_WARN_ON_USE_PREPARE([[#include <uchar.h>
    ]], [c32rtomb mbrtoc32])
])

dnl On Haiku 2020, char16_t and char32_t are incorrectly defined.
dnl See <https://dev.haiku-os.org/ticket/15990>.
AC_DEFUN_ONCE([gl_TYPE_CHAR16_T],
[
  AC_REQUIRE([gl_UCHAR_H_DEFAULTS])
  dnl Determine whether gnulib's <uchar.h> would, if present, override char16_t.
  AC_CACHE_CHECK([whether char16_t is correctly defined],
    [gl_cv_type_char16_t_works],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
          #include <uchar.h>
          /* For simplicity, assume that uint16_least_t is equivalent to
             'unsigned short'.  */
          int verify[(char16_t)(-1) >= 0 && sizeof (char16_t) == sizeof (unsigned short) ? 1 : -1];
          ]])
       ],
       [gl_cv_type_char16_t_works=yes],
       [gl_cv_type_char16_t_works=no])
    ])
  if test $gl_cv_type_char16_t_works = no; then
    GNULIB_OVERRIDES_CHAR16_T=1
  fi
])
AC_DEFUN_ONCE([gl_TYPE_CHAR32_T],
[
  AC_REQUIRE([gl_UCHAR_H_DEFAULTS])
  dnl Determine whether gnulib's <uchar.h> would, if present, override char32_t.
  AC_CACHE_CHECK([whether char32_t is correctly defined],
    [gl_cv_type_char32_t_works],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
          #include <uchar.h>
          /* For simplicity, assume that uint32_least_t is equivalent to
             'unsigned int'.  */
          int verify[(char32_t)(-1) >= 0 && sizeof (char32_t) == sizeof (unsigned int) ? 1 : -1];
          ]])
       ],
       [gl_cv_type_char32_t_works=yes],
       [gl_cv_type_char32_t_works=no])
    ])
  if test $gl_cv_type_char32_t_works = no; then
    GNULIB_OVERRIDES_CHAR32_T=1
  fi
])

AC_DEFUN([gl_UCHAR_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_UCHAR_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_UCHAR_H_DEFAULTS],
[
  GNULIB_OVERRIDES_CHAR16_T=0; AC_SUBST([GNULIB_OVERRIDES_CHAR16_T])
  GNULIB_OVERRIDES_CHAR32_T=0; AC_SUBST([GNULIB_OVERRIDES_CHAR32_T])
  GNULIB_BTOC32=0;             AC_SUBST([GNULIB_BTOC32])
  GNULIB_C32ISALNUM=0;         AC_SUBST([GNULIB_C32ISALNUM])
  GNULIB_C32ISALPHA=0;         AC_SUBST([GNULIB_C32ISALPHA])
  GNULIB_C32ISBLANK=0;         AC_SUBST([GNULIB_C32ISBLANK])
  GNULIB_C32ISCNTRL=0;         AC_SUBST([GNULIB_C32ISCNTRL])
  GNULIB_C32ISDIGIT=0;         AC_SUBST([GNULIB_C32ISDIGIT])
  GNULIB_C32ISGRAPH=0;         AC_SUBST([GNULIB_C32ISGRAPH])
  GNULIB_C32ISLOWER=0;         AC_SUBST([GNULIB_C32ISLOWER])
  GNULIB_C32ISPRINT=0;         AC_SUBST([GNULIB_C32ISPRINT])
  GNULIB_C32ISPUNCT=0;         AC_SUBST([GNULIB_C32ISPUNCT])
  GNULIB_C32ISSPACE=0;         AC_SUBST([GNULIB_C32ISSPACE])
  GNULIB_C32ISUPPER=0;         AC_SUBST([GNULIB_C32ISUPPER])
  GNULIB_C32ISXDIGIT=0;        AC_SUBST([GNULIB_C32ISXDIGIT])
  GNULIB_C32RTOMB=0;           AC_SUBST([GNULIB_C32RTOMB])
  GNULIB_C32SNRTOMBS=0;        AC_SUBST([GNULIB_C32SNRTOMBS])
  GNULIB_C32SRTOMBS=0;         AC_SUBST([GNULIB_C32SRTOMBS])
  GNULIB_C32STOMBS=0;          AC_SUBST([GNULIB_C32STOMBS])
  GNULIB_C32TOB=0;             AC_SUBST([GNULIB_C32TOB])
  GNULIB_MBRTOC32=0;           AC_SUBST([GNULIB_MBRTOC32])
  GNULIB_MBSNRTOC32S=0;        AC_SUBST([GNULIB_MBSNRTOC32S])
  GNULIB_MBSRTOC32S=0;         AC_SUBST([GNULIB_MBSRTOC32S])
  GNULIB_MBSTOC32S=0;          AC_SUBST([GNULIB_MBSTOC32S])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_C32RTOMB=1;             AC_SUBST([HAVE_C32RTOMB])
  HAVE_MBRTOC32=1;             AC_SUBST([HAVE_MBRTOC32])
  REPLACE_C32RTOMB=0;          AC_SUBST([REPLACE_C32RTOMB])
  REPLACE_MBRTOC32=0;          AC_SUBST([REPLACE_MBRTOC32])
])
