/* Test of flock() function.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <sys/file.h>

#include "signature.h"
SIGNATURE_CHECK (flock, int, (int, int));

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "macros.h"

static void
test_shared (const char *file, int fd)
{
  /* Should be able to acquire several shared locks on a file, through
   * different file table entries.
   */
  int fd2, r;

  ASSERT (flock (fd, LOCK_SH) == 0);

  fd2 = open (file, O_RDWR, 0644);
  ASSERT (fd2 >= 0);

  r = flock (fd2, LOCK_SH | LOCK_NB);
  ASSERT (r == 0);              /* Was able to acquire a second shared lock. */

  ASSERT (flock (fd, LOCK_UN) == 0);
  ASSERT (close (fd2) == 0);
}

static void
test_exclusive (const char *file, int fd)
{
  /* Should not be able to acquire more than one exclusive lock on a file. */
  int fd2, r;

  ASSERT (flock (fd, LOCK_EX) == 0);

  fd2 = open (file, O_RDWR, 0644);
  ASSERT (fd2 >= 0);

  r = flock (fd2, LOCK_EX | LOCK_NB);
  ASSERT (r == -1);             /* Was unable to acquire a second exclusive lock. */

#if 0
  /* The Linux manual page of flock(2) says:
       "A process may only hold one type of lock (shared or exclusive) on a
       file. Subsequent flock() calls on an already locked file will convert
       an existing lock to the new lock mode."
     So, the call below should convert the exclusive lock for fd to a shared
     and thus succeeds.  The fact that it doesn't but instead fails is
     apparently a bug.  */
  /* The Solaris manual page of flock(2) says:
       "More than one process may hold a shared lock for a file at any given
        time, but multiple exclusive, or both shared and exclusive, locks may
        not exist simultaneously on a file. ...
        Requesting a lock on an object that is already locked normally causes
        the caller to block until the lock may be acquired. If LOCK_NB is
        included in operation, then this will not happen; instead, the call
        will fail and the error EWOULDBLOCK will be returned."
     So, the call below should fail and set errno to EWOULDBLOCK.  The fact
     that it succeeds is apparently a bug.  */
  r = flock (fd2, LOCK_SH | LOCK_NB);
  ASSERT (r == -1);
#endif

  ASSERT (flock (fd, LOCK_UN) == 0);
  ASSERT (close (fd2) == 0);
}

int
main (int argc, char *argv[])
{
  int fd;
  const char *file = "test-flock.txt";

  /* Open a non-empty file for testing. */
  fd = open (file, O_RDWR | O_CREAT | O_TRUNC, 0644);
  ASSERT (fd >= 0);
  ASSERT (write (fd, "hello", 5) == 5);

#if defined __linux__ || defined __ANDROID__
  /* Invalid operation codes are rejected by the Linux implementation and by
     the gnulib replacement,  but not by the Mac OS X implementation.  */
  ASSERT (flock (fd, LOCK_SH | LOCK_EX) == -1);
  ASSERT (errno == EINVAL);
  ASSERT (flock (fd, LOCK_SH | LOCK_UN) == -1);
  ASSERT (errno == EINVAL);
  ASSERT (flock (fd, LOCK_EX | LOCK_UN) == -1);
  ASSERT (errno == EINVAL);
  ASSERT (flock (fd, 0) == -1);
  ASSERT (errno == EINVAL);
#endif

  test_shared (file, fd);
  test_exclusive (file, fd);

  /* Close and remove the test file. */
  ASSERT (close (fd) == 0);
  ASSERT (unlink (file) == 0);

  return 0;
}
