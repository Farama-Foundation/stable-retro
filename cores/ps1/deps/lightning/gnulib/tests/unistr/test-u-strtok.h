/* Test of uN_strtok() functions.
   Copyright (C) 2015-2021 Free Software Foundation, Inc.

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

static void
test_u_strtok (void)
{
  {
    UNIT input[] = { 'f', 'o', 'o', 0 };
    const UNIT delim[] = { 0 };
    UNIT *state;
    const UNIT *result = U_STRTOK (input, delim, &state);
    ASSERT (result == input);
  }

  {
    UNIT input[] =
      { 'A', 'B', 'C', ' ', 'A', 'B', 'C', 'D', 'A', 'B', ' ', '\t',
        'A', 'B', 'C', 'D', 'A', 'B', 'C', 'D', 'A', 'B', 'D', 'E', 0
      };
    const UNIT delim[] = { ' ', '\t', 0 };
    UNIT *state;
    const UNIT *result;
    result = U_STRTOK (input, delim, &state);
    ASSERT (result == input);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == input + 4);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == input + 12);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == NULL);
  }

  /* Check for multibyte delimiters.  */
  {
    ucs4_t u_input[] =
      { 'A', 'B', 'C', 0x3000, 'A', 'B', 'C', 'D', 'A', 'B', 0x3000, 0x3001,
        'A', 'B', 'C', 'D', 'A', 'B', 'C', 'D', 'A', 'B', 'D', 'E', 0
      };
    ucs4_t u_delim[] = { 0x3000, 0x3001, 0 };
    size_t input_len = 6 * SIZEOF (u_input);
    UNIT *input = (UNIT *) malloc (input_len);
    size_t delim_len = 6 * SIZEOF (u_delim);
    UNIT *delim = (UNIT *) malloc (delim_len);
    UNIT *state;
    const UNIT *result;
    UNIT *ptr, *first_ptr, *second_ptr;
    size_t i;
    for (i = 0, ptr = input; i < SIZEOF (u_input) && u_input[i] != 0; i++)
      {
        int ret = U_UCTOMB (ptr, u_input[i], input_len - (ptr - input));
        if (i == 4)
          first_ptr = ptr;
        if (i == 12)
          second_ptr = ptr;
        ptr += ret;
      }
    *ptr = 0;
    for (i = 0, ptr = delim; i < SIZEOF (u_delim) && u_delim[i] != 0; i++)
      {
        int ret = U_UCTOMB (ptr, u_delim[i], delim_len - (ptr - delim));
        ptr += ret;
      }
    *ptr = 0;
    result = U_STRTOK (input, delim, &state);
    ASSERT (result == input);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == first_ptr);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == second_ptr);
    result = U_STRTOK (NULL, delim, &state);
    ASSERT (result == NULL);
    free (input);
    free (delim);
  }
}
