#!/bin/sh

# Test against a memory leak.

(${CHECKER} ./test-dprintf-posix2${EXEEXT} 0
 result=$?
 if test $result != 77 && test $result != 78; then result=1; fi
 exit $result
) 2>/dev/null
malloc_result=$?
if test $malloc_result = 77; then
  echo "Skipping test: no way to determine address space size"
  exit 77
fi

${CHECKER} ./test-dprintf-posix2${EXEEXT} 1 > /dev/null
result=$?
if test $result = 77; then
  echo "Skipping test: no way to determine address space size"
  exit 77
fi
if test $result != 0; then
  exit 1
fi

if test $malloc_result = 78; then
  echo "Skipping test: get_rusage_as() doesn't work"
  exit 77
fi

exit 0
