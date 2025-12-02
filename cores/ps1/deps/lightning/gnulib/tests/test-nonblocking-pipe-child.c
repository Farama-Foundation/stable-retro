/* Child program invoked by test-nonblocking-pipe-main.

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

#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "binary-io.h"

#include "macros.h"
#include "test-nonblocking-pipe.h"
#define PROG_ROLE "child"
#include "test-nonblocking-reader.h"

int
main (int argc, char *argv[])
{
  int test = atoi (argv[1]);

  /* Close unused file descriptors.  */
  close (STDOUT_FILENO);

  /* STDIN_FILENO was created as binary in the parent process.  But since an
     fd's mode is stored in the process, not in the kernel, on native Windows
     we need to set it as binary in the child process again.  */
  set_binary_mode (STDIN_FILENO, O_BINARY);

  main_reader_loop (test, PIPE_DATA_BLOCK_SIZE, STDIN_FILENO);

  return 0;
}
