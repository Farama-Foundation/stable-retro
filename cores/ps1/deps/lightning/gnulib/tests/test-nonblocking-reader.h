/* The reader part of a test program for non-blocking communication.

   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

/* This program implements 4 tests:

   test == 0:
     Test blocking write() with blocking read().

     Timeline         Main process             Child process
        0 s           Start                    Start, read(10000)
        1 s           write(20000)             Return from read(10000)
        2 s                                    Next read(10000)
        2 s           Return from write(20000) Return from read(10000)

   test == 1:
     Test non-blocking write() with blocking read().

     Timeline         Main process             Child process
        0 s           Start                    Start, read(10000)
        1 s           write(20000)             Return from read(10000)
                      Return with at least 10000,
                      Repeatedly continue
                      write() of the rest
        2 s                                    Next read(10000)
        2 s           Return from write(10000) Return from read(10000)

   test == 2:
     Test blocking write() with non-blocking read().

     Timeline         Main process             Child process
        0 s           Start                    Start, read(10000)
                                               repeatedly polling
        1 s           write(20000)             Return from read(10000)
        2 s                                    Next read(10000)
        2 s           Return from write(20000) Return from read(10000)

   test == 3:
     Test non-blocking write() with non-blocking read().
 */

#include "test-nonblocking-misc.h"

static ssize_t
full_read (size_t fd, void *buf, size_t count)
{
  size_t bytes_read;

  bytes_read = 0;
  while (bytes_read < count)
    {
      TIMING_DECLS
      ssize_t ret;
      int saved_errno;

      dbgfprintf (stderr, "%s: >> read (%lu)\n", PROG_ROLE,
                  (unsigned long) (count - bytes_read));
      START_TIMING
      ret = read (fd, (char *) buf + bytes_read, count - bytes_read);
      saved_errno = errno;
      END_TIMING
      dbgfprintf (stderr, "%s: << read -> %ld%s\n", PROG_ROLE,
                  (long) ret, dbgstrerror (ret < 0, saved_errno));
      if (ret < 0)
        return -1;
      else
        {
          ASSERT (ret > 0);
          bytes_read += ret;
        }
    }
  return bytes_read;
}

static ssize_t
full_read_from_nonblocking_fd (size_t fd, void *buf, size_t count)
{
  size_t bytes_read;

  bytes_read = 0;
  while (bytes_read < count)
    {
      TIMING_DECLS
      ssize_t ret;
      int saved_errno;

      dbgfprintf (stderr, "%s: >> read (%lu)\n", PROG_ROLE,
                  (unsigned long) (count - bytes_read));
      START_TIMING
      ret = read (fd, (char *) buf + bytes_read, count - bytes_read);
      saved_errno = errno;
      END_TIMING
      dbgfprintf (stderr, "%s: << read -> %ld%s\n", PROG_ROLE,
                  (long) ret, dbgstrerror (ret < 0, saved_errno));
      /* This assertion fails if the non-blocking flag is effectively not set
         on fd.  */
      ASSERT (spent_time < 0.5);
      if (ret < 0)
        {
          ASSERT (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
          usleep (SMALL_DELAY);
        }
      else
        {
          ASSERT (ret > 0);
          bytes_read += ret;
        }
    }
  return bytes_read;
}

/* Execute the reader loop.  */
static void
main_reader_loop (int test, size_t data_block_size, int fd)
{
  unsigned char *expected;
  unsigned char *data;

  /* Set up the expected data.  */
  expected = init_data (data_block_size);

  data = (unsigned char *) malloc (2 * data_block_size);
  ASSERT (data != NULL);

  switch (test)
    {
    TIMING_DECLS
    ssize_t ret;

    case 0: /* Test blocking write() with blocking read().  */
    case 1: /* Test non-blocking write() with blocking read().  */
      START_TIMING
      ret = full_read (fd, data, data_block_size);
      END_TIMING
      ASSERT (ret == data_block_size);
      ASSERT (memcmp (data, expected, data_block_size) == 0);
      ASSERT (spent_time > 0.5);
      /* This assertion fails if data_block_size is very large and
         ENABLE_DEBUGGING is 1.  */
      ASSERT (spent_time < 1.5);

      usleep (1000000);

      START_TIMING
      ret = full_read (fd, data, data_block_size);
      END_TIMING
      ASSERT (ret == data_block_size);
      ASSERT (memcmp (data, expected + data_block_size, data_block_size) == 0);
      /* This assertion fails if data_block_size is much larger than needed
         and SMALL_DELAY is too large.  */
      ASSERT (spent_time < 0.5);

      break;

    case 2: /* Test blocking write() with non-blocking read().  */
    case 3: /* Test non-blocking write() with non-blocking read().  */
      START_TIMING
      ret = full_read_from_nonblocking_fd (fd, data, data_block_size);
      END_TIMING
      ASSERT (ret == data_block_size);
      ASSERT (memcmp (data, expected, data_block_size) == 0);
      ASSERT (spent_time > 0.5);
      /* This assertion fails if data_block_size is much larger than needed
         and SMALL_DELAY is too large, or if data_block_size is very large and
         ENABLE_DEBUGGING is 1.  */
      ASSERT (spent_time < 1.5);

      usleep (1000000);

      START_TIMING
      ret = full_read_from_nonblocking_fd (fd, data, data_block_size);
      END_TIMING
      ASSERT (ret == data_block_size);
      ASSERT (memcmp (data, expected + data_block_size, data_block_size) == 0);
      /* This assertion fails if data_block_size is much larger than needed
         and SMALL_DELAY is too large.  */
      ASSERT (spent_time < 0.5);

      break;

    default:
      abort ();
    }

  free (data);
  free (expected);
}
