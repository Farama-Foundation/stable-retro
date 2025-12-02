#!/bin/sh

# Test against a memory leak.

(${CHECKER} ./test-fprintf-posix3${EXEEXT} 0
 result=$?
 if test $result != 77 && test $result != 78 && test $result != 80; then result=1; fi
 exit $result
) 2>/dev/null
malloc_result=$?
if test $malloc_result = 77; then
  echo "Skipping test: no way to determine address space size"
  exit 77
fi
if test $malloc_result = 78; then
  echo "Skipping test: cannot trust address space size on this platform"
  exit 77
fi

${CHECKER} ./test-fprintf-posix3${EXEEXT} 1 > /dev/null
result=$?
if test $result = 77; then
  echo "Skipping test: no way to determine address space size"
  exit 77
fi
if test $result != 0; then
  exit 1
fi

exit 0
