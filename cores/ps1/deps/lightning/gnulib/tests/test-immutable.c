/* Test of immutable data.
   Copyright (C) 2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2021.  */

#include <config.h>

#include "immutable.h"

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

struct data
{
  int x;
  long y;
};

#if IMMUTABLE_EFFECTIVE

static _GL_ASYNC_SAFE _Noreturn void
segv_handler (int signo)
{
  _Exit (0);
}

static void
install_segv_handler (void)
{
  signal (SIGSEGV, segv_handler);
# if (defined __APPLE__ && defined __MACH__) || defined __FreeBSD__
  signal (SIGBUS, segv_handler);
# endif
}

#endif

int
main (int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf (stderr, "%s: need 1 argument\n", argv[0]);
      return 1;
    }
  int test = atoi (argv[1]);
  switch (test)
    {
    case 0:
      /* Indicates whether the implementation effectively rejects writes to
         immutable data.  */
      #if !IMMUTABLE_EFFECTIVE
      fputs ("Skipping test: immutability cannot be enforced\n", stderr);
      return 77;
      #else
      break;
      #endif

    case 1:
      /* Correct use of immmalloc.  */
      {
        struct data *wp;
        struct data const *p;

        wp = (struct data *) immmalloc (sizeof (struct data));
        ASSERT (wp != NULL);
        wp->x = 7;
        wp->y = 42;
        p = immfreeze (wp);
        ASSERT (p->x == 7);
        ASSERT (p->y == 42);
        immfree (p);
      }
      break;

    case 2:
      /* Catch invalid write access.  */
      {
        struct data *wp;
        struct data const *p;

        wp = (struct data *) immmalloc (sizeof (struct data));
        ASSERT (wp != NULL);
        wp->x = 7;
        wp->y = 42;
        p = immfreeze (wp);
        #if IMMUTABLE_EFFECTIVE
        install_segv_handler ();
        #endif
        /* This assignment should crash.  */
        ((struct data *) p)->y = 77;
        #if IMMUTABLE_EFFECTIVE
        return 1;
        #endif
      }
      break;

    case 3:
      /* Catch invalid write access while another data object is not frozen.  */
      {
        struct data *wp;
        struct data const *p;
        struct data *wp2;

        wp = (struct data *) immmalloc (sizeof (struct data));
        ASSERT (wp != NULL);
        wp->x = 7;
        wp->y = 42;
        p = immfreeze (wp);
        ASSERT (p->x == 7);
        ASSERT (p->y == 42);

        wp2 = (struct data *) immmalloc (sizeof (struct data));
        ASSERT (wp2 != NULL);
        wp2->x = 7;
        #if IMMUTABLE_EFFECTIVE
        install_segv_handler ();
        #endif
        /* This assignment should crash.  */
        ((struct data *) p)->y = 42;
        #if IMMUTABLE_EFFECTIVE
        return 1;
        #endif
      }
      break;

    case 4:
      /* Correct use of immstrdup.  */
      {
        const char *s = immstrdup ("Hello");
        ASSERT (strlen (s) == 5);
        ASSERT (strcmp (s, "Hello") == 0);
        immfree (s);
      }
      break;

    default:
      ASSERT (false);
    }
  return 0;
}
