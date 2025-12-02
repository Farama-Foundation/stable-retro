#!/bin/sh
: ${srcdir=.}
. "$srcdir/init.sh"; path_prepend_ .

too_big=99999999999999999999999999999999999999999999999999999999999999999999
result=0

# test xstrtoll
${CHECKER} test-xstrtoll 1 >> out 2>&1 || result=1
${CHECKER} test-xstrtoll -1 >> out 2>&1 || result=1
${CHECKER} test-xstrtoll 1k >> out 2>&1 || result=1
${CHECKER} test-xstrtoll ${too_big}h >> out 2>&1 && result=1
${CHECKER} test-xstrtoll $too_big >> out 2>&1 && result=1
${CHECKER} test-xstrtoll x >> out 2>&1 && result=1
${CHECKER} test-xstrtoll 9x >> out 2>&1 && result=1
${CHECKER} test-xstrtoll 010 >> out 2>&1 || result=1
# suffix without integer is valid
${CHECKER} test-xstrtoll MiB >> out 2>&1 || result=1

# test xstrtoull
${CHECKER} test-xstrtoull 1 >> out 2>&1 || result=1
${CHECKER} test-xstrtoull -1 >> out 2>&1 && result=1
${CHECKER} test-xstrtoull 1k >> out 2>&1 || result=1
${CHECKER} test-xstrtoull ${too_big}h >> out 2>&1 && result=1
${CHECKER} test-xstrtoull $too_big >> out 2>&1 && result=1
${CHECKER} test-xstrtoull x >> out 2>&1 && result=1
${CHECKER} test-xstrtoull 9x >> out 2>&1 && result=1
${CHECKER} test-xstrtoull 010 >> out 2>&1 || result=1
${CHECKER} test-xstrtoull MiB >> out 2>&1 || result=1

# Find out how to remove carriage returns from output. Solaris /usr/ucb/tr
# does not understand '\r'.
if echo solaris | tr -d '\r' | grep solais > /dev/null; then
  cr='\015'
else
  cr='\r'
fi

# normalize output
LC_ALL=C tr -d "$cr" < out > k
mv k out

# compare expected output
cat > expected <<EOF
1->1 ()
-1->-1 ()
1k->1024 ()
invalid suffix in X argument '${too_big}h'
X argument '$too_big' too large
invalid X argument 'x'
invalid suffix in X argument '9x'
010->8 ()
MiB->1048576 ()
1->1 ()
invalid X argument '-1'
1k->1024 ()
invalid suffix in X argument '${too_big}h'
X argument '$too_big' too large
invalid X argument 'x'
invalid suffix in X argument '9x'
010->8 ()
MiB->1048576 ()
EOF

compare expected out || result=1

Exit $result
