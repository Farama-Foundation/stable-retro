/* Test for nonblocking read and write.

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

/* Whether to print debugging messages.  */
#define ENABLE_DEBUGGING 0

/* Delay (in microseconds) to sleep when write() or read() returned -1 with
   errno = EAGAIN.  */
#define SMALL_DELAY 10000

/* Return a memory area, filled with the data to be transferred.  */
static unsigned char *
init_data (size_t data_block_size)
{
  unsigned char *data;
  unsigned int i;

  data = (unsigned char *) malloc (2 * data_block_size);
  ASSERT (data != NULL);

  for (i = 0; i < 2 * data_block_size; i++)
    data[i] = (unsigned char) (i * i + (7 * i) % 61 + 4);

  return data;
}

#if ENABLE_DEBUGGING
# include <stdarg.h>
static int dbgfprintf (FILE *fp, const char *format, ...)
                      _GL_ATTRIBUTE_FORMAT_PRINTF_STANDARD (2, 3);
static int
dbgfprintf (FILE *fp, const char *format, ...)
{
  /* Accumulate the entire line in a buffer, so that the output on fp
     is done atomically.  */
  char line[1024];
  size_t line_len;
  struct timeval current_time;
  va_list args;
  int ret;

  line_len = 0;
  gettimeofday (&current_time, NULL);
  ret = snprintf (line, sizeof (line), "%.6f ",
                  current_time.tv_sec + (double) current_time.tv_usec * 1e-6);
  if (ret < 0)
    return -1;
  line_len = strlen (line);

  va_start (args, format);
  ret = vsnprintf (line + line_len, sizeof (line) - line_len, format, args);
  va_end (args);
  if (ret < 0)
    return -1;
  line_len += strlen (line + line_len);

  ret = fwrite (line, 1, line_len, fp);

  /* Make sure the debugging information is output, so that the order of the
     messages reflects the timeline of events, and so that the output is not
     lost if the program crashes afterwards (relevant on mingw).  */
  fflush (fp);
  return ret;
}
#else
# define dbgfprintf if (1) ; else fprintf
#endif

/* Return a textual description of the error code ERR, if FAILED is true.
   Return an empty string if FAILED is false.  */
static const char *
dbgstrerror (bool failed, int err)
{
  static char buf[256];
  if (failed)
    {
      sprintf (buf, " %d %s", err, strerror (err));
      return buf;
    }
  else
    return "";
}

#define TIMING_DECLS \
  struct timeval before_time; \
  struct timeval after_time; \
  double spent_time;
#define START_TIMING \
  gettimeofday (&before_time, NULL);
#define END_TIMING \
  gettimeofday (&after_time, NULL); \
  spent_time = \
    (after_time.tv_sec - before_time.tv_sec) \
    + ((double) after_time.tv_usec - (double) before_time.tv_usec) * 1e-6;
