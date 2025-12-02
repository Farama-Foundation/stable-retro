#!/bin/sh

# A POSIX compliant 'id' program.
if test -f /usr/xpg4/bin/id; then
  ID=/usr/xpg4/bin/id
else
  ID=id
fi

# The user's original uid and gid.
uid=`$ID -u`
gid=`$ID -g`

if test `$ID -u` = 0; then
  # No need to ask for a password.
  "${srcdir}/test-idpriv-droptemp.su.sh" "$uid" "$gid"
else
  echo "Need root privileges for the 'test-idpriv-droptemp' test. Deny if you don't trust." > /dev/tty
  if (type sudo) > /dev/null 2>&1; then
    # Use the 'sudo' program.
    sudo "${srcdir}/test-idpriv-droptemp.su.sh" "$uid" "$gid"
  else
    # Use the 'su' program.
    su root -c "${srcdir}/test-idpriv-droptemp.su.sh \"$uid\" \"$gid\""
  fi
  result=$?
  case $result in
    0)  exit 0;;
    77) exit 77;;
    13) exit 1;;
    *)  echo "Skipping test: root privileges not granted"; exit 77;;
  esac
fi
