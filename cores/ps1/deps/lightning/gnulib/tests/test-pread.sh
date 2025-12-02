#!/bin/sh
. "${srcdir=.}/init.sh"; path_prepend_ .

fail=0
: | ${CHECKER} test-pread || fail=1

Exit $fail
