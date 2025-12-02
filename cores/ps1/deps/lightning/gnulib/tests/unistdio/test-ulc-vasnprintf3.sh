#!/bin/sh

# Test whether a french Unicode locale is installed.
: ${LOCALE_FR_UTF8=fr_FR.UTF-8}
if test $LOCALE_FR_UTF8 != none; then
  testlocale=$LOCALE_FR_UTF8
else
  if test -f /usr/bin/localedef; then
    echo "Skipping test: no french Unicode locale is installed"
  else
    echo "Skipping test: no french Unicode locale is supported"
  fi
  exit 77
fi

LC_ALL=$testlocale \
${CHECKER} ./test-ulc-vasnprintf1${EXEEXT} \
  || exit 1

LC_ALL=$testlocale \
${CHECKER} ./test-ulc-vasnprintf3${EXEEXT}
