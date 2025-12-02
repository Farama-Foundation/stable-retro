# locale-ar.m4 serial 9
dnl Copyright (C) 2003, 2005-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Ben Pfaff, based on locale-fr.m4 by Bruno Haible.

dnl Determine the name of an Arabic locale with traditional encoding.
AC_DEFUN([gt_LOCALE_AR],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_CACHE_CHECK([for a traditional Arabic locale], [gt_cv_locale_ar], [
    AC_LANG_CONFTEST([AC_LANG_SOURCE([[
#include <locale.h>
#include <time.h>
#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif
#include <stdlib.h>
#include <string.h>
struct tm t;
char buf[16];
int main () {
  /* On BeOS and Haiku, locales are not implemented in libc.  Rather, libintl
     imitates locale dependent behaviour by looking at the environment
     variables, and all locales use the UTF-8 encoding.  */
#if defined __BEOS__ || defined __HAIKU__
  return 1;
#else
  /* Check whether the given locale name is recognized by the system.  */
# if defined _WIN32 && !defined __CYGWIN__
  /* On native Windows, setlocale(category, "") looks at the system settings,
     not at the environment variables.  Also, when an encoding suffix such
     as ".65001" or ".54936" is specified, it succeeds but sets the LC_CTYPE
     category of the locale to "C".  */
  if (setlocale (LC_ALL, getenv ("LC_ALL")) == NULL
      || strcmp (setlocale (LC_CTYPE, NULL), "C") == 0)
    return 1;
# else
  if (setlocale (LC_ALL, "") == NULL) return 1;
# endif
  /* Check that nl_langinfo(CODESET) is nonempty and not "ASCII" or "646"
     and ends in "6". */
# if HAVE_LANGINFO_CODESET
  {
    const char *cs = nl_langinfo (CODESET);
    if (cs[0] == '\0' || strcmp (cs, "ASCII") == 0 || strcmp (cs, "646") == 0
        || cs[strlen (cs) - 1] != '6')
      return 1;
  }
# endif
# ifdef __CYGWIN__
  /* On Cygwin, avoid locale names without encoding suffix, because the
     locale_charset() function relies on the encoding suffix.  Note that
     LC_ALL is set on the command line.  */
  if (strchr (getenv ("LC_ALL"), '.') == NULL) return 1;
# endif
  return 0;
#endif
}
      ]])])
    if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
      case "$host_os" in
        # Handle native Windows specially, because there setlocale() interprets
        # "ar" as "Arabic" or "Arabic_Saudi Arabia.1256",
        # "fr" or "fra" as "French" or "French_France.1252",
        # "ge"(!) or "deu"(!) as "German" or "German_Germany.1252",
        # "ja" as "Japanese" or "Japanese_Japan.932",
        # and similar.
        mingw*)
          # Note that on native Windows, the Arabic locale is
          # "Arabic_Saudi Arabia.1256", and CP1256 is very different from
          # ISO-8859-6, so we cannot use it here.
          gt_cv_locale_ar=none
          ;;
        *)
          # Setting LC_ALL is not enough. Need to set LC_TIME to empty, because
          # otherwise on Mac OS X 10.3.5 the LC_TIME=C from the beginning of the
          # configure script would override the LC_ALL setting. Likewise for
          # LC_CTYPE, which is also set at the beginning of the configure script.
          # Values tested:
          #   - The usual locale name:                         ar_SA
          #   - The locale name with explicit encoding suffix: ar_SA.ISO-8859-6
          #   - The HP-UX locale name:                         ar_SA.iso88596
          #   - The Solaris 7 locale name:                     ar
          # Also try ar_EG instead of ar_SA because Egypt is a large country too.
          for gt_cv_locale_ar in ar_SA ar_SA.ISO-8859-6 ar_SA.iso88596 ar_EG ar_EG.ISO-8859-6 ar_EG.iso88596 ar none; do
            if test $gt_cv_locale_ar = none; then
              break
            fi
            if (LC_ALL=$gt_cv_locale_ar LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
              break
            fi
          done
          ;;
      esac
    fi
    rm -fr conftest*
  ])
  LOCALE_AR=$gt_cv_locale_ar
  AC_SUBST([LOCALE_AR])
])
