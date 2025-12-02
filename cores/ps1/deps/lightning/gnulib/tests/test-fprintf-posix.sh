#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="$tmpfiles t-fprintf-posix.tmp t-fprintf-posix.out"
${CHECKER} ./test-fprintf-posix${EXEEXT} > t-fprintf-posix.tmp || exit 1
LC_ALL=C tr -d '\r' < t-fprintf-posix.tmp > t-fprintf-posix.out || exit 1

: ${DIFF=diff}
${DIFF} "${srcdir}/test-printf-posix.output" t-fprintf-posix.out
result=$?

rm -fr $tmpfiles

exit $result
