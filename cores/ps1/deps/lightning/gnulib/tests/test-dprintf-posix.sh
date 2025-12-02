#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="$tmpfiles t-dprintf-posix.tmp t-dprintf-posix.out"
${CHECKER} ./test-dprintf-posix${EXEEXT} > t-dprintf-posix.tmp || exit 1
LC_ALL=C tr -d '\r' < t-dprintf-posix.tmp > t-dprintf-posix.out || exit 1

: ${DIFF=diff}
${DIFF} "${srcdir}/test-printf-posix.output" t-dprintf-posix.out
result=$?

rm -fr $tmpfiles

exit $result
