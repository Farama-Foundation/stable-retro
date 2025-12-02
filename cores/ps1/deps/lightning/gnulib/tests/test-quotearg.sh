#!/bin/sh

# Choose an existing locale. The locale encoding does not matter; see the
# comment in test-quotearg.po.
if test $LOCALE_FR_UTF8 != none; then
  locale=$LOCALE_FR_UTF8
else
  if test $LOCALE_FR != none; then
    locale=$LOCALE_FR
  else
    locale=none
  fi
fi

# Work around a limitation of gettext() on native Windows in gettext 0.18.1:
# the locale identifiers in the environment variables LC_ALL etc. have to
# be in Unix conventions, not in native Window conventions.
if test $locale = French_France.65001; then
  locale=fr_FR.UTF-8
fi
if test $locale = French_France.1252; then
  locale=fr_FR.CP1252
fi

# Work around a bug on Solaris 11 systems with no GNU gettext installed.
# See gettext/gettext-tools/tests/init.cfg.
localedir="$srcdir/testlocale"
if test $locale != none && test $locale != fr; then
  case "$host_os" in
    solaris2.11)
      mkdir -p testlocale
      cp -R "$srcdir/testlocale/fr" "testlocale/$locale"
      localedir="testlocale"
      ;;
  esac
fi

LOCALE=$locale LOCALEDIR="$localedir" \
${CHECKER} ./test-quotearg${EXEEXT}
result=$?

if test $locale != none && test $locale != fr; then
  case "$host_os" in
    solaris2.11)
      rm -rf "testlocale/$locale"
      rmdir testlocale 2>/dev/null
      ;;
  esac
fi

exit $result
