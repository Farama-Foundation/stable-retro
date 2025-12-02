#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="$tmpfiles t-tsearch.out"
${CHECKER} ./test-tsearch${EXEEXT} > t-tsearch.out 2>&1
test $? = 0 || { cat t-tsearch.out 1>&2; rm -f $tmpfiles; exit 1; }

rm -f $tmpfiles

exit 0
