#!/bin/sh

# Test blocking write() with blocking read().
${CHECKER} ./test-nonblocking-socket-main${EXEEXT} ./test-nonblocking-socket-child${EXEEXT} 0 || exit 1

# Test non-blocking write() with blocking read().
${CHECKER} ./test-nonblocking-socket-main${EXEEXT} ./test-nonblocking-socket-child${EXEEXT} 1 || exit 1

# Test blocking write() with non-blocking read().
${CHECKER} ./test-nonblocking-socket-main${EXEEXT} ./test-nonblocking-socket-child${EXEEXT} 2 || exit 1

# Test non-blocking write() with non-blocking read().
${CHECKER} ./test-nonblocking-socket-main${EXEEXT} ./test-nonblocking-socket-child${EXEEXT} 3 || exit 1
