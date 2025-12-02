#!/bin/sh
exec ${CHECKER} ./test-u32-nfkc-big${EXEEXT} "$srcdir/uninorm/NormalizationTest.txt"
