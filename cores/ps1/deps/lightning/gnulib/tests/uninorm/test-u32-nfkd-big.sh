#!/bin/sh
exec ${CHECKER} ./test-u32-nfkd-big${EXEEXT} "$srcdir/uninorm/NormalizationTest.txt"
