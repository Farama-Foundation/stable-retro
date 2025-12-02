#!/bin/sh

# Note: This test fails on Cygwin 1.5.x, because the non-blocking flag has
# apparently no effect on STDOUT_FILENO. It is fixed in Cygwin 1.7.

# Test blocking write() with blocking read().
# Commented out because this test succeeds on all platforms anyway.
#${CHECKER} ./test-nonblocking-pipe-main${EXEEXT} ./test-nonblocking-pipe-child${EXEEXT} 0 || exit 1

# Test non-blocking write() with blocking read().
${CHECKER} ./test-nonblocking-pipe-main${EXEEXT} ./test-nonblocking-pipe-child${EXEEXT} 1 || exit 1

# Test blocking write() with non-blocking read().
${CHECKER} ./test-nonblocking-pipe-main${EXEEXT} ./test-nonblocking-pipe-child${EXEEXT} 2 || exit 1

# Test non-blocking write() with non-blocking read().
${CHECKER} ./test-nonblocking-pipe-main${EXEEXT} ./test-nonblocking-pipe-child${EXEEXT} 3 || exit 1
