#!/bin/sh

# Show all commands when run with environment variable VERBOSE=yes.
test -z "$VERBOSE" || set -x
prog=test-parse-duration

exe=`pwd`/${prog}${EXEEXT}
nl='
'

# func_tmpdir
# creates a temporary directory.
# Sets variable
# - tmp             pathname of freshly created temporary directory
func_tmpdir ()
{
  # Use the environment variable TMPDIR, falling back to /tmp. This allows
  # users to specify a different temporary directory, for example, if their
  # /tmp is filled up or too small.
  : ${TMPDIR=/tmp}
  {
    # Use the mktemp program if available. If not available, hide the error
    # message.
    tmp=`(umask 077 && mktemp -d "$TMPDIR/glXXXXXX") 2>/dev/null` &&
    test -n "$tmp" && test -d "$tmp"
  } ||
  {
    # Use a simple mkdir command. It is guaranteed to fail if the directory
    # already exists.  $RANDOM is bash specific and expands to empty in shells
    # other than bash, ksh and zsh.  Its use does not increase security;
    # rather, it minimizes the probability of failure in a very cluttered /tmp
    # directory.
    tmp=$TMPDIR/gl$$-$RANDOM
    (umask 077 && mkdir "$tmp")
  } ||
  {
    echo "$0: cannot create a temporary directory in $TMPDIR" >&2
    exit 1
  }
}

die ()
{
  echo "${prog} fatal error:  $*" >&2
  exit 1
}

func_tmpdir
trap 'rm -rf "${tmp}"' EXIT
tmpf="${tmp}/tests.txt"

cat > "${tmpf}" <<- _EOF_
	1 Y 2 M 3 W 4 d 5 h 6 m 7 s
	P 00010225 T 05:06:07
	P 1Y2M3W4D T 5H6M7S
	1 Y 2 M 25 D 5:6:7
	1 Y 2 M 25 d 5h 6:7
	1 Y 2 M 25 d 5h 6m 7
	P 1-2-25 T 5:6:7
	_EOF_

exec 3< "${tmpf}"
while read line <&3
do
    v=`${CHECKER} ${exe} "${line}"` || { ls -l "${tmpf}"; die "Failed: ${exe} '${line}'"; }
    test $v -eq 38898367 || die $v is not 38898367
done
exec 3>&-
