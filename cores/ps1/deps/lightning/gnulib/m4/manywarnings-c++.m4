# manywarnings-c++.m4 serial 3
dnl Copyright (C) 2008-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Implementation of the specialization of gl_MANYWARN_ALL_GCC
# for _AC_LANG = C++.
AC_DEFUN([gl_MANYWARN_ALL_GCC_CXX_IMPL],
[
  AC_LANG_PUSH([C++])

  dnl First, check for some issues that only occur when combining multiple
  dnl gcc warning categories.
  AC_REQUIRE([AC_PROG_CXX])
  if test -n "$GXX"; then

    dnl Check if -W -Werror -Wno-missing-field-initializers is supported
    dnl with the current $CXX $CXXFLAGS $CPPFLAGS.
    AC_CACHE_CHECK([whether -Wno-missing-field-initializers is supported],
      [gl_cv_cxx_nomfi_supported],
      [gl_save_CXXFLAGS="$CXXFLAGS"
       CXXFLAGS="$CXXFLAGS -W -Werror -Wno-missing-field-initializers"
       AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]], [[]])],
         [gl_cv_cxx_nomfi_supported=yes],
         [gl_cv_cxx_nomfi_supported=no])
       CXXFLAGS="$gl_save_CXXFLAGS"
      ])

    if test "$gl_cv_cxx_nomfi_supported" = yes; then
      dnl Now check whether -Wno-missing-field-initializers is needed
      dnl for the { 0, } construct.
      AC_CACHE_CHECK([whether -Wno-missing-field-initializers is needed],
        [gl_cv_cxx_nomfi_needed],
        [gl_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS -W -Werror"
         AC_COMPILE_IFELSE(
           [AC_LANG_PROGRAM(
              [[int f (void)
                {
                  typedef struct { int a; int b; } s_t;
                  s_t s1 = { 0, };
                  return s1.b;
                }
              ]],
              [[]])],
           [gl_cv_cxx_nomfi_needed=no],
           [gl_cv_cxx_nomfi_needed=yes])
         CXXFLAGS="$gl_save_CXXFLAGS"
        ])
    fi

    dnl Next, check if -Werror -Wuninitialized is useful with the
    dnl user's choice of $CXXFLAGS; some versions of gcc warn that it
    dnl has no effect if -O is not also used
    AC_CACHE_CHECK([whether -Wuninitialized is supported],
      [gl_cv_cxx_uninitialized_supported],
      [gl_save_CXXFLAGS="$CXXFLAGS"
       CXXFLAGS="$CXXFLAGS -Werror -Wuninitialized"
       AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]], [[]])],
         [gl_cv_cxx_uninitialized_supported=yes],
         [gl_cv_cxx_uninitialized_supported=no])
       CXXFLAGS="$gl_save_CXXFLAGS"
      ])

  fi

  # List all gcc warning categories.
  # To compare this list to your installed GCC's, run this Bash command:
  #
  # comm -3 \
  #  <(sed -n 's/^  *\(-[^ ]*\) .*/\1/p' manywarnings-c++.m4 | sort) \
  #  <(gcc --help=warnings | sed -n 's/^  \(-[^ ]*\) .*/\1/p' | sort |
  #      grep -v -x -f <(
  #         awk '/^[^#]/ {print $1}' ../build-aux/g++-warning.spec))

  gl_manywarn_set=
  for gl_manywarn_item in \
    -W \
    -Wabi-tag \
    -Waddress \
    -Waggressive-loop-optimizations \
    -Wall \
    -Wattributes \
    -Wbool-compare \
    -Wbuiltin-macro-redefined \
    -Wcast-align \
    -Wchar-subscripts \
    -Wclobbered \
    -Wcomment \
    -Wcomments \
    -Wconditionally-supported \
    -Wconversion-null \
    -Wcoverage-mismatch \
    -Wcpp \
    -Wctor-dtor-privacy \
    -Wdate-time \
    -Wdelete-incomplete \
    -Wdelete-non-virtual-dtor \
    -Wdeprecated \
    -Wdeprecated-declarations \
    -Wdisabled-optimization \
    -Wdiv-by-zero \
    -Wdouble-promotion \
    -Weffc++ \
    -Wempty-body \
    -Wendif-labels \
    -Wenum-compare \
    -Wextra \
    -Wformat-contains-nul \
    -Wformat-extra-args \
    -Wformat-nonliteral \
    -Wformat-security \
    -Wformat-signedness \
    -Wformat-y2k \
    -Wformat-zero-length \
    -Wfree-nonheap-object \
    -Wignored-qualifiers \
    -Winherited-variadic-ctor \
    -Winit-self \
    -Winline \
    -Wint-to-pointer-cast \
    -Winvalid-memory-model \
    -Winvalid-offsetof \
    -Winvalid-pch \
    -Wliteral-suffix \
    -Wlogical-not-parentheses \
    -Wlogical-op \
    -Wmain \
    -Wmaybe-uninitialized \
    -Wmemset-transposed-args \
    -Wmissing-braces \
    -Wmissing-declarations \
    -Wmissing-field-initializers \
    -Wmissing-include-dirs \
    -Wmultichar \
    -Wnarrowing \
    -Wnoexcept \
    -Wnon-template-friend \
    -Wnon-virtual-dtor \
    -Wnonnull \
    -Wodr \
    -Wold-style-cast \
    -Wopenmp-simd \
    -Woverflow \
    -Woverlength-strings \
    -Woverloaded-virtual \
    -Wpacked \
    -Wpacked-bitfield-compat \
    -Wparentheses \
    -Wpmf-conversions \
    -Wpointer-arith \
    -Wpragmas \
    -Wreorder \
    -Wreturn-local-addr \
    -Wreturn-type \
    -Wsequence-point \
    -Wshadow \
    -Wshift-count-negative \
    -Wshift-count-overflow \
    -Wsign-promo \
    -Wsized-deallocation \
    -Wsizeof-array-argument \
    -Wsizeof-pointer-memaccess \
    -Wstack-protector \
    -Wstrict-aliasing \
    -Wstrict-null-sentinel \
    -Wstrict-overflow \
    -Wsuggest-attribute=const \
    -Wsuggest-attribute=format \
    -Wsuggest-attribute=noreturn \
    -Wsuggest-attribute=pure \
    -Wsuggest-final-methods \
    -Wsuggest-final-types \
    -Wsuggest-override \
    -Wswitch \
    -Wswitch-bool \
    -Wsync-nand \
    -Wsystem-headers \
    -Wtrampolines \
    -Wtrigraphs \
    -Wtype-limits \
    -Wuninitialized \
    -Wunknown-pragmas \
    -Wunsafe-loop-optimizations \
    -Wunused \
    -Wunused-but-set-parameter \
    -Wunused-but-set-variable \
    -Wunused-function \
    -Wunused-label \
    -Wunused-local-typedefs \
    -Wunused-macros \
    -Wunused-parameter \
    -Wunused-result \
    -Wunused-value \
    -Wunused-variable \
    -Wuseless-cast \
    -Wvarargs \
    -Wvariadic-macros \
    -Wvector-operation-performance \
    -Wvirtual-move-assign \
    -Wvla \
    -Wvolatile-register-var \
    -Wwrite-strings \
    -Wzero-as-null-pointer-constant \
    \
    ; do
    gl_manywarn_set="$gl_manywarn_set $gl_manywarn_item"
  done

  # gcc --help=warnings outputs an unusual form for these options; list
  # them here so that the above 'comm' command doesn't report a false match.
  gl_manywarn_set="$gl_manywarn_set -Warray-bounds=2"
  gl_manywarn_set="$gl_manywarn_set -Wnormalized=nfc"
  gl_manywarn_set="$gl_manywarn_set -Wshift-overflow=2"
  gl_manywarn_set="$gl_manywarn_set -Wunused-const-variable=2"

  # These are needed for older GCC versions.
  if test -n "$GXX"; then
    case `($CXX --version) 2>/dev/null` in
      'g++ (GCC) '[[0-3]].* | \
      'g++ (GCC) '4.[[0-7]].*)
        gl_manywarn_set="$gl_manywarn_set -fdiagnostics-show-option"
        gl_manywarn_set="$gl_manywarn_set -funit-at-a-time"
          ;;
    esac
  fi

  # Disable specific options as needed.
  if test "$gl_cv_cxx_nomfi_needed" = yes; then
    gl_manywarn_set="$gl_manywarn_set -Wno-missing-field-initializers"
  fi

  if test "$gl_cv_cxx_uninitialized_supported" = no; then
    gl_manywarn_set="$gl_manywarn_set -Wno-uninitialized"
  fi

  $1=$gl_manywarn_set

  AC_LANG_POP([C++])
])
