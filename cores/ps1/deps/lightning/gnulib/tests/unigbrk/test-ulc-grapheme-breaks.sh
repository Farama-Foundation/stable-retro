#!/bin/sh

# Test in an ISO-8895-6 locale.
: ${LOCALE_AR=ar_SA}
if test $LOCALE_AR = none; then
  if test -f /usr/bin/localedef; then
    echo "Skipping test: no traditional Arabic locale is installed"
  else
    echo "Skipping test: no traditional Arabic locale is supported"
  fi
  exit 77
fi

LC_ALL=$LOCALE_AR \
${CHECKER} ./test-ulc-grapheme-breaks${EXEEXT}
