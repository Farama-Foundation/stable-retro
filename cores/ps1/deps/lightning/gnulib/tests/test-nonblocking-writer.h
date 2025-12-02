/* The writer part of a test program for non-blocking communication.

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

/* Execute the writer loop.
   Returns 0 if successful, 1 if data_block_size is too small.  */
static int
main_writer_loop (int test, size_t data_block_size, int fd,
                  bool has_large_buffer)
{
  int too_small = 0;
  unsigned char *data;

  /* Set up the data to transfer.  */
  data = init_data (data_block_size);

  switch (test)
    {
    TIMING_DECLS
    ssize_t ret;

    case 0: /* Test blocking write() with blocking read().  */
    case 2: /* Test blocking write() with non-blocking read().  */
      {
        int saved_errno;

        usleep (1000000);

        dbgfprintf (stderr, "%s:1: >> write (%lu)\n", PROG_ROLE,
                    (unsigned long) (2 * data_block_size));
        START_TIMING
        ret = write (fd, data, 2 * data_block_size);
        saved_errno = errno;
        END_TIMING
        dbgfprintf (stderr, "%s:1: << write -> %ld%s\n", PROG_ROLE,
                    (long) ret, dbgstrerror (ret < 0, saved_errno));
        ASSERT (ret == 2 * data_block_size);
        if (!has_large_buffer)
          {
            /* This assertion fails if data_block_size is too small.  */
            if (!(spent_time > 0.5))
              {
                fprintf (stderr,
                         "%s:1: spent_time = %g, data_block_size too small\n",
                         PROG_ROLE, spent_time);
                too_small = 1;
              }
          }
        ASSERT (spent_time < 1.5);
      }
      break;

    case 1: /* Test non-blocking write() with blocking read().  */
    case 3: /* Test non-blocking write() with non-blocking read().  */
      {
        size_t bytes_written;
        int saved_errno;

        usleep (1000000);

        bytes_written = 0;
        while (bytes_written < 2 * data_block_size)
          {
            dbgfprintf (stderr, "%s:2: >> write (%lu)\n", PROG_ROLE,
                        (unsigned long) (2 * data_block_size - bytes_written));
            START_TIMING
            ret = write (fd, data + bytes_written,
                         2 * data_block_size - bytes_written);
            saved_errno = errno;
            END_TIMING
            dbgfprintf (stderr, "%s:2: << write -> %ld%s\n", PROG_ROLE,
                        (long) ret, dbgstrerror (ret < 0, saved_errno));
            if (ret < 0 && bytes_written >= data_block_size)
              {
                ASSERT (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
                ASSERT (spent_time < 0.5);
                break;
              }
            /* This assertion fails if the non-blocking flag is effectively not
               set on fd.  */
            ASSERT (spent_time < 0.5);
            if (ret < 0)
              {
                ASSERT (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
                usleep (SMALL_DELAY);
              }
            else
              {
                /* This assertion fails if data_block_size is too small.  */
                if (!(ret > 0))
                  {
                    fprintf (stderr,
                             "%s:1: spent_time = %g, data_block_size too small\n",
                             PROG_ROLE, spent_time);
                    too_small = 1;
                  }
                bytes_written += ret;
              }
          }
        ASSERT (bytes_written >= data_block_size);

        while (bytes_written < 2 * data_block_size)
          {
            dbgfprintf (stderr, "%s:3: >> write (%lu)\n", PROG_ROLE,
                        (unsigned long) (2 * data_block_size - bytes_written));
            START_TIMING
            ret = write (fd, data + bytes_written,
                         2 * data_block_size - bytes_written);
            saved_errno = errno;
            END_TIMING
            dbgfprintf (stderr, "%s:3: << write -> %ld%s\n", PROG_ROLE,
                        (long) ret, dbgstrerror (ret < 0, saved_errno));
            ASSERT (spent_time < 0.5);
            if (ret < 0)
              {
                ASSERT (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
                usleep (SMALL_DELAY);
              }
            else
              {
                ASSERT (ret > 0);
                bytes_written += ret;
              }
          }
      }
      break;

    default:
      abort ();
    }

  free (data);
  return too_small;
}
