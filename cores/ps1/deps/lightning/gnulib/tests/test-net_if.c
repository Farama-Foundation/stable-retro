/* Test of <net/if.h> functions.
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

/* Written by Simon Josefsson <simon@josefsson.org>, 2010.  */

#include <config.h>

#include <net/if.h>

/* We do not yet have replacements for if_* functions on systems that
   lack a native <net/if.h>.  */
#if HAVE_NET_IF_H && HAVE_IF_NAMEINDEX
# include "signature.h"
SIGNATURE_CHECK (if_freenameindex, void, (struct if_nameindex *));
SIGNATURE_CHECK (if_indextoname, char *, (unsigned int, char *));
SIGNATURE_CHECK (if_nameindex, struct if_nameindex *, (void));
SIGNATURE_CHECK (if_nametoindex, unsigned int, (const char *));
#endif

#include <stddef.h> /* NULL */
#include <stdio.h> /* fprintf */

int
main (int argc, char *argv[])
{
#if HAVE_NET_IF_H && HAVE_IF_NAMEINDEX
  struct if_nameindex *ifnp, *p;

  p = ifnp = if_nameindex ();
  if (ifnp == NULL)
    {
      fputs ("if_nameindex returned NULL\n", stderr);
      return 1;
    }

  while (p->if_index)
    {
      unsigned int idx;
      char buf[IF_NAMESIZE];
      char *q;

      if (argc > 1)
        printf ("index %d name %s\n", p->if_index, p->if_name);

      idx = if_nametoindex (p->if_name);
      if (idx != p->if_index)
        {
          fprintf (stderr, "if_nametoindex (%s) = %d != %d\n",
                   p->if_name, idx, p->if_index);
          return 1;
        }

      q = if_indextoname (p->if_index, buf);
      if (q == NULL)
        {
          fprintf (stderr, "if_indextoname (%d) returned NULL\n", p->if_index);
          return 1;
        }
      if (q != buf)
        {
          fprintf (stderr, "if_indextoname (%d) buffer mismatch?\n",
                   p->if_index);
          return 1;
        }
      if (strcmp (p->if_name, q) != 0)
        {
          fprintf (stderr, "if_indextoname (%s) = %s ?!\n", p->if_name, q);
          return 1;
        }

      p++;
    }

  if_freenameindex (ifnp);
#endif /* HAVE_NET_IF_H */

#if !HAVE_NET_IF_H || HAVE_IF_NAMEINDEX
  {
    static struct if_nameindex ni;
    return !IF_NAMESIZE + ni.if_index + !!ni.if_name;
  }
#else
  return 0;
#endif
}
