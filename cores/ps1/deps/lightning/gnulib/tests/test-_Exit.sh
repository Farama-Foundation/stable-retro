#!/bin/sh

${CHECKER} ./test-_Exit${EXEEXT}
test $? = 81
