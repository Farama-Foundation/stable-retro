/* Test harness for pipe-filter-gi.

   Copyright (C) 2009-2021 Free Software Foundation, Inc.
   Written by Paolo Bonzini <bonzini@gnu.org>, 2009.

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

#include "pipe-filter.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "full-write.h"
#include "macros.h"

/* 0.1 sec pause */
static void
small_nap (void)
{
  usleep (100000);
}

static char static_buf[5];

static void *
prepare_read (size_t *num_bytes_p, void *private_data)
{
  *num_bytes_p = sizeof (static_buf);
  return static_buf;
}

/* Callback that ignores the data that has been read.  */

static void
ignore_done_read (void *data_read, size_t num_bytes_read, void *private_data)
{
}

/* Callback that outputs the data that has been read.  */

static void
output_done_read (void *data_read, size_t num_bytes_read, void *private_data)
{
  full_write (STDOUT_FILENO, data_read, num_bytes_read);
}

static void
pipe_filter_gi_write_string (struct pipe_filter_gi *f, const char *string)
{
  pipe_filter_gi_write (f, string, strlen (string));
}

int
main (int argc, char **argv)
{
  struct pipe_filter_gi *f;
  const char *path[] = { NULL, NULL };

  ASSERT (argc == 2);

  /* Test writing to a nonexistent program traps sooner or later.  */
  {
    int rc;

    path[0] = "/nonexistent/blah";
    f = pipe_filter_gi_create ("pipe-filter-test", path[0], path, true, false,
                               prepare_read, ignore_done_read, NULL);
    small_nap ();
    rc = pipe_filter_gi_write (f, "", 1);
    ASSERT (rc == 127 || rc == -1);
    rc = pipe_filter_gi_close (f);
    ASSERT (rc == 127 || rc == -1);
    printf ("Test 1 passed.\n");
    fflush (stdout);
  }

  /* Test returning the exit status.  */
  {
    path[0] = argv[1];
    f = pipe_filter_gi_create ("pipe-filter-test", path[0], path, false, false,
                               prepare_read, ignore_done_read, NULL);
    pipe_filter_gi_write_string (f, "1 -1");
    ASSERT (pipe_filter_gi_close (f) == 1);
    printf ("Test 2 passed.\n");
    fflush (stdout);
  }

  /* Same, but test returning the status in pipe_filter_gi_write.  */
  {
    int rc;

    path[0] = argv[1];
    f = pipe_filter_gi_create ("pipe-filter-test", path[0], path, false, false,
                               prepare_read, ignore_done_read, NULL);
    pipe_filter_gi_write_string (f, "1 -1\n"); /* tell the child to terminate */
    small_nap (); /* let the child terminate */
    rc = pipe_filter_gi_write (f, " ", 1); /* write to a closed pipe */
    ASSERT (rc == -1 && errno == EPIPE);
    /* Closing the filter must report same error again, for consistency with
       pipe_filter_ii_execute.  The subprocess' exit status is not returned.  */
    rc = pipe_filter_gi_close (f);
    ASSERT (rc == -1 && errno == EPIPE);
    printf ("Test 3 passed.\n");
    fflush (stdout);
  }

  /* Now test asynchronous I/O.  */
  {
    path[0] = argv[1];
    f = pipe_filter_gi_create ("pipe-filter-test", path[0], path, false, true,
                               prepare_read, output_done_read, NULL);
    pipe_filter_gi_write_string (f, "1 50\n");
    pipe_filter_gi_write_string (f, "51\n");
    pipe_filter_gi_write_string (f, "100");
    ASSERT (pipe_filter_gi_close (f) == 0);
    printf ("Test 4 passed.\n");
    fflush (stdout);
  }

  return 0;
}
