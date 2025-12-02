#!/bin/sh
# This script must be run as superuser.

origuid=$1
origgid=$2

# A POSIX compliant 'id' program.
if test -f /usr/xpg4/bin/id; then
  ID=/usr/xpg4/bin/id
else
  ID=id
fi

if test `$ID -u` != 0; then
  echo "Skipping test: not superuser"
  exit 77
fi

${CHECKER} ./test-idpriv-drop${EXEEXT} || exit 13          # normal

chown root:root test-idpriv-drop${EXEEXT} 2>/dev/null || \
chown root:wheel test-idpriv-drop${EXEEXT} 2>/dev/null || \
  { echo "Skipping test: root privilege not sufficient on this file system"
    exit 77
  }
chmod 4755 test-idpriv-drop${EXEEXT}
./test-idpriv-drop${EXEEXT} || exit 13          # setuid root
chmod 2755 test-idpriv-drop${EXEEXT}
./test-idpriv-drop${EXEEXT} || exit 13          # setgid root
chmod 6755 test-idpriv-drop${EXEEXT}
./test-idpriv-drop${EXEEXT} || exit 13          # setuid and setgid root

if chown nobody test-idpriv-drop${EXEEXT} 2>/dev/null; then
  chmod 4755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setuid nobody
  chmod 2755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setgid root
  chmod 6755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setuid nobody and setgid root
fi

if chown root:nobody test-idpriv-drop${EXEEXT} 2>/dev/null; then
  chmod 4755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setuid root
  chmod 2755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setgid nobody
  chmod 6755 test-idpriv-drop${EXEEXT}
  ./test-idpriv-drop${EXEEXT} || exit 13        # setuid root and setgid nobody
fi

chown "$origuid:$origgid" test-idpriv-drop${EXEEXT}
chmod 755 test-idpriv-drop${EXEEXT}
