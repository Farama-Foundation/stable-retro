#!/bin/sh

# Test whether a traditional french locale is installed.
: ${LOCALE_FR=fr_FR}
if test $LOCALE_FR != none; then
  testlocale=$LOCALE_FR
else
  if test -f /usr/bin/localedef; then
    echo "Skipping test: no traditional french locale is installed"
  else
    echo "Skipping test: no traditional french locale is supported"
  fi
  exit 77
fi

LC_ALL=$testlocale \
${CHECKER} ./test-ulc-vasnprintf1${EXEEXT} \
  || exit 1

LC_ALL=$testlocale \
${CHECKER} ./test-ulc-vasnprintf2${EXEEXT}
