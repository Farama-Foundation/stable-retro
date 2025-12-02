/* Test the fts function.
   Copyright 2017-2021 Free Software Foundation, Inc.

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

#include <fts_.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BASE "t-fts.tmp"
static char base[] = BASE; /* Not const, since argv needs non-const.  */
static char const base_d[] = BASE "/d";
static char *const argv[2] = { base, 0 };

static void
perror_exit (char const *message, int status)
{
  perror (message);
  exit (status);
}

/* alloc/dealloc to ensure structures initialized appropriately.  */
static void
fts_dealloc (void)
{
  static char dir[] = "./";
  static char *const curr_dir[2] = { dir, 0 };
  FTS *ftsp = fts_open (curr_dir, FTS_NOSTAT | FTS_PHYSICAL | FTS_CWDFD, 0);
  if (ftsp)
    {
      if (fts_close (ftsp) != 0)
        perror_exit ("fts_close", 9);
    }
  else
    perror_exit (base, 10);
}

/* Remove BASE and all files under it.  */
static void
remove_tree (void)
{
  FTSENT *e;
  FTS *ftsp = fts_open (argv, FTS_NOSTAT | FTS_PHYSICAL | FTS_CWDFD, 0);
  if (ftsp)
    {
      while ((e = fts_read (ftsp)))
        {
          int status = 0;
          switch (e->fts_info)
            {
            case FTS_DP:
              status = unlinkat (ftsp->fts_cwd_fd, e->fts_accpath,
                                 AT_REMOVEDIR);
              break;

            case FTS_F: case FTS_NSOK:
              status = unlinkat (ftsp->fts_cwd_fd, e->fts_accpath, 0);
              break;
            }
          if (status != 0)
            perror_exit (e->fts_path, 1);
        }
      if (fts_close (ftsp) != 0)
        perror_exit ("fts_close", 2);
    }
  else if (errno != ENOENT)
    perror_exit (base, 3);
}

int
main (void)
{
  FTS *ftsp;
  FTSENT *e;
  char buf[sizeof BASE + 100];
  int i;
  enum { needles = 4 };
  int needles_seen = 0;
  struct stat st;

  remove_tree ();

  /* Create directories BASE, BASE/d, BASE/d/1, BASE/d/2, ..., BASE/d/65536,
     to stress-test fts.  Stop if directory creation fails due to
     EMFILE or EMLINK problems, or if BASE/d's link count no longer matches the
     Unix tradition.  See:
     https://bugzilla.kernel.org/show_bug.cgi?id=196405
     for more info.  */
  if (mkdir (BASE, 0777) != 0)
    perror_exit (base, 4);
  if (mkdir (base_d, 0777) != 0)
    perror_exit (base_d, 5);
  for (i = 1; i <= 65536; i++)
    {
      sprintf (buf, "%s/d/%i", base, i);
      if (mkdir (buf, 0777) != 0)
        {
          if (errno == EMFILE || errno == EMLINK)
            break;
          if (i <= needles)
            perror_exit (buf, 77);
          break;
        }
      if (needles < i && stat (base_d, &st) == 0 && st.st_nlink != i + 2)
        break;
    }

  /* Create empty files BASE/d/1/needle etc.  */
  for (i = 1; i <= needles; i++)
    {
      int fd;
      sprintf (buf, "%s/d/%d/needle", base, i);
      fd = open (buf, O_WRONLY | O_CREAT, 0666);
      if (fd < 0 || close (fd) != 0)
        perror_exit (buf, 77);
    }

  /* Use fts to look for the needles.  */
  ftsp = fts_open (argv, FTS_SEEDOT | FTS_NOSTAT | FTS_PHYSICAL | FTS_CWDFD, 0);
  if (!ftsp)
    perror_exit (base, 6);
  while ((e = fts_read (ftsp)))
    needles_seen += strcmp (e->fts_name, "needle") == 0;
  int fts_read_errno = errno;
  fflush (stdout);
  if (fts_read_errno)
    {
      errno = fts_read_errno;
      perror_exit ("fts_read", 7);
    }
  if (fts_close (ftsp) != 0)
    perror_exit (base, 8);

  /* Report an error if we did not find the needles.  */
  if (needles_seen != needles)
    {
      fprintf (stderr, "%d needles found (should be %d)\n",
               needles_seen, needles);
      return 1;
    }

  remove_tree ();
  if (stat (base, &st) == 0)
    {
      fprintf (stderr, "fts could not remove directory\n");
      return 1;
    }

  fts_dealloc ();

  return 0;
}
