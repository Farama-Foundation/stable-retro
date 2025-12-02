#!/bin/sh

# Print "Skipping test" if immutability cannot be enforced.
./test-immutable${EXEEXT} 0
if test $? = 77; then exit 77; fi

st=0
for i in 1 2 3 4 ; do
  ${CHECKER} ./test-immutable${EXEEXT} $i \
    || { echo test-immutable.sh: test case $i failed >&2; st=1; }
done
exit $st
