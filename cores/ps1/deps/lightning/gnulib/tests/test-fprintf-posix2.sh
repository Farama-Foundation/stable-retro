#!/bin/sh

# Test out-of-memory handling.

(${CHECKER} ./test-fprintf-posix2${EXEEXT} 0
 result=$?
 if test $result != 77 && test $result != 78; then result=1; fi
 exit $result
) 2>/dev/null
malloc_result=$?
if test $malloc_result = 77; then
  echo "Skipping test: getrlimit and setrlimit don't work"
  exit 77
fi

for arg in 1 2 3 4 5 6
do
  ${CHECKER} ./test-fprintf-posix2${EXEEXT} $arg > /dev/null
  result=$?
  if test $result = 77; then
    echo "Skipping test: getrlimit and setrlimit don't work"
    exit 77
  fi
  if test $result != 0; then
    exit 1
  fi
done

if test $malloc_result = 78; then
  echo "Skipping test: getrlimit and setrlimit don't work"
  exit 77
fi

exit 0
