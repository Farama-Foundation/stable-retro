# obstack-printf-posix.m4 serial 4
dnl Copyright (C) 2008-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_OBSTACK_PRINTF_POSIX],
[
  dnl Persuade glibc <stdio.h> to declare obstack_printf(), obstack_vprintf().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_PRINTF_SIZES_C99])
  AC_REQUIRE([gl_PRINTF_LONG_DOUBLE])
  AC_REQUIRE([gl_PRINTF_INFINITE])
  AC_REQUIRE([gl_PRINTF_INFINITE_LONG_DOUBLE])
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_A])
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_F])
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_N])
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_LS])
  AC_REQUIRE([gl_PRINTF_POSITIONS])
  AC_REQUIRE([gl_PRINTF_FLAG_GROUPING])
  AC_REQUIRE([gl_PRINTF_FLAG_LEFTADJUST])
  AC_REQUIRE([gl_PRINTF_FLAG_ZERO])
  AC_REQUIRE([gl_PRINTF_PRECISION])
  AC_REQUIRE([gl_PRINTF_ENOMEM])
  gl_cv_func_obstack_printf_posix=no
  AC_CHECK_FUNCS_ONCE([obstack_printf])
  if test $ac_cv_func_obstack_printf = yes ; then
    case "$gl_cv_func_printf_sizes_c99" in
      *yes)
        case "$gl_cv_func_printf_long_double" in
          *yes)
            case "$gl_cv_func_printf_infinite" in
              *yes)
                case "$gl_cv_func_printf_infinite_long_double" in
                  *yes)
                    case "$gl_cv_func_printf_directive_a" in
                      *yes)
                        case "$gl_cv_func_printf_directive_f" in
                          *yes)
                            case "$gl_cv_func_printf_directive_n" in
                              *yes)
                                case "$gl_cv_func_printf_directive_ls" in
                                  *yes)
                                    case "$gl_cv_func_printf_positions" in
                                      *yes)
                                        case "$gl_cv_func_printf_flag_grouping" in
                                          *yes)
                                            case "$gl_cv_func_printf_flag_leftadjust" in
                                              *yes)
                                                case "$gl_cv_func_printf_flag_zero" in
                                                  *yes)
                                                    case "$gl_cv_func_printf_precision" in
                                                      *yes)
                                                        case "$gl_cv_func_printf_enomem" in
                                                          *yes)
                                                            # obstack_printf exists and is
                                                            # already POSIX compliant.
                                                            gl_cv_func_obstack_printf_posix=yes
                                                            ;;
                                                        esac
                                                        ;;
                                                    esac
                                                    ;;
                                                esac
                                                ;;
                                            esac
                                            ;;
                                        esac
                                        ;;
                                    esac
                                    ;;
                                esac
                                ;;
                            esac
                            ;;
                        esac
                        ;;
                    esac
                    ;;
                esac
                ;;
            esac
            ;;
        esac
        ;;
    esac
  fi
  if test $gl_cv_func_obstack_printf_posix = no; then
    gl_PREREQ_VASNPRINTF_LONG_DOUBLE
    gl_PREREQ_VASNPRINTF_INFINITE_DOUBLE
    gl_PREREQ_VASNPRINTF_INFINITE_LONG_DOUBLE
    gl_PREREQ_VASNPRINTF_DIRECTIVE_A
    gl_PREREQ_VASNPRINTF_DIRECTIVE_F
    gl_PREREQ_VASNPRINTF_DIRECTIVE_LS
    gl_PREREQ_VASNPRINTF_FLAG_GROUPING
    gl_PREREQ_VASNPRINTF_FLAG_LEFTADJUST
    gl_PREREQ_VASNPRINTF_FLAG_ZERO
    gl_PREREQ_VASNPRINTF_PRECISION
    gl_PREREQ_VASNPRINTF_ENOMEM
    gl_REPLACE_VASNPRINTF
    gl_REPLACE_OBSTACK_PRINTF
  fi
  gl_DECL_OBSTACK_PRINTF
])
