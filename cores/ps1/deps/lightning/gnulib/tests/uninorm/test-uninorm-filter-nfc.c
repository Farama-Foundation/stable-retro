/* Test of canonical normalization of streams.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "uninorm.h"

#include <stdlib.h>

#include "unistr.h"
#include "macros.h"

/* A stream of Unicode characters that simply accumulates the contents.  */

struct accumulator
{
  uint32_t *result;
  size_t length;
  size_t allocated;
};

static int
write_to_accumulator (void *stream_data, ucs4_t uc)
{
  struct accumulator *accu = (struct accumulator *) stream_data;

  if (accu->length == accu->allocated)
    {
      accu->allocated = 2 * accu->allocated + 1;
      accu->result = (uint32_t *) realloc (accu->result, accu->allocated * sizeof (uint32_t));
    }
  accu->result[accu->length] = uc;
  accu->length++;
  return 0;
}

static int
check (const uint32_t *input, size_t input_length,
       const uint32_t *expected, size_t expected_length)
{
  struct accumulator accu;
  struct uninorm_filter *filter;
  size_t i;

  accu.result = NULL;
  accu.length = 0;
  accu.allocated = 0;

  filter = uninorm_filter_create (UNINORM_NFC, write_to_accumulator, &accu);
  ASSERT (filter != NULL);

  for (i = 0; i < input_length; i++)
    ASSERT (uninorm_filter_write (filter, input[i]) == 0);

  ASSERT (uninorm_filter_free (filter) == 0);

  if (!(accu.result != NULL))
    return 1;
  if (!(accu.length == expected_length))
    return 2;
  if (!(u32_cmp (accu.result, expected, expected_length) == 0))
    return 3;
  free (accu.result);

  return 0;
}

int
main ()
{
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint32_t input[] =
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint32_t decomposed[] =
      { 'G', 'r', 0x0075, 0x0308, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0438, 0x0306, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',',
        0x1112, 0x1161, 0x11AB, 0x1100, 0x1173, 0x11AF, '\n'
      };
    ASSERT (check (input, SIZEOF (input),           input, SIZEOF (input)) == 0);
    ASSERT (check (decomposed, SIZEOF (decomposed), input, SIZEOF (input)) == 0);
  }

  return 0;
}
