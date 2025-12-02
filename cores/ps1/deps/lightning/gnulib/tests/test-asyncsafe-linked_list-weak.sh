#!/bin/sh

st=0
for i in 1 2 3 ; do
  ${CHECKER} ./test-asyncsafe-linked_list-weak${EXEEXT} $i
  result=$?
  if test $result = 77; then
    st=77
    break
  fi
  if test $result != 0; then
    echo "test-asyncsafe-linked_list-weak.sh: test case $i failed" >&2
    st=1
  fi
done
exit $st
