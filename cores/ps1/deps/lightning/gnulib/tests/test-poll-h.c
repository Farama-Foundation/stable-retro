/* Test of <poll.h> substitute.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible, 2010.  */

#include <config.h>

#include <poll.h>

/* Check that the nfds_t type is defined.  */
nfds_t a;

/* Check that the various POLL* macros are defined.  */
int b = POLLIN | POLLPRI | POLLOUT | POLLERR | POLLHUP | POLLNVAL
        | POLLRDNORM | POLLRDBAND | POLLWRNORM | POLLWRBAND;

int
main (void)
{
  return 0;
}
