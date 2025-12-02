/* Test of bitset.
   Copyright (C) 2018-2021 Free Software Foundation, Inc.

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

#include "bitset.h"

#include "macros.h"

#define RANDOM(n) (rand () % (n))

static void
assert_bitset_equal (bitset bs1, bitset bs2)
{
  debug_bitset (bs1);
  debug_bitset (bs2);
  ASSERT (bitset_size (bs1) == bitset_size (bs2));
  for (bitset_bindex i = 0; i < bitset_size (bs1); ++i)
    ASSERT (bitset_test (bs1, i) == bitset_test (bs2, i));
}

static void
bitset_random (bitset bs)
{
  for (bitset_bindex i = 0; i < bitset_size (bs); ++i)
    bitset_set (bs, RANDOM (2));
}


/* Check various operations on random bitsets with two different
   implementations.  */

static void
compare (enum bitset_attr a, enum bitset_attr b)
{
  /* bitset_list (used in many operations) uses a cache whose size is
     BITSET_LIST_SIZE */
  const int nbits = RANDOM (2 * BITSET_LIST_SIZE);

  /* Four read only random initial values of type A.  */
  const bitset asrc0 = bitset_create (nbits, a);
  bitset_random (asrc0);
  const bitset asrc1 = bitset_create (nbits, a);
  bitset_random (asrc1);
  const bitset asrc2 = bitset_create (nbits, a);
  bitset_random (asrc2);
  const bitset asrc3 = bitset_create (nbits, a);
  bitset_random (asrc3);

  /* Four read only values of type B, equal to the values of type A. */
  const bitset bsrc0 = bitset_create (nbits, b);
  bitset_copy (bsrc0, asrc0);
  const bitset bsrc1 = bitset_create (nbits, b);
  bitset_copy (bsrc1, asrc1);
  const bitset bsrc2 = bitset_create (nbits, b);
  bitset_copy (bsrc2, asrc2);
  const bitset bsrc3 = bitset_create (nbits, b);
  bitset_copy (bsrc3, asrc3);

  /* Destinations for each operation.  */
  bitset adst = bitset_create (nbits, a);
  bitset bdst = bitset_create (nbits, b);

  /* count */
  ASSERT (bitset_count (asrc0) == bitset_count (bsrc0));

  /* not */
  bitset_not (adst, asrc0);
  bitset_not (bdst, bsrc0);
  assert_bitset_equal (adst, bdst);

  /* and */
  bitset_and (adst, asrc0, asrc1);
  bitset_and (bdst, bsrc0, bsrc1);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_and_cmp (adst, asrc0, asrc1)
          == bitset_and_cmp (bdst, bsrc0, bsrc1));
  assert_bitset_equal (adst, bdst);

  /* andn */
  bitset_andn (adst, asrc0, asrc1);
  bitset_andn (bdst, bsrc0, bsrc1);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_andn_cmp (adst, asrc0, asrc1)
          == bitset_andn_cmp (bdst, bsrc0, bsrc1));
  assert_bitset_equal (adst, bdst);

  /* or */
  bitset_or (adst, asrc0, asrc1);
  bitset_or (bdst, bsrc0, bsrc1);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_or_cmp (adst, asrc0, asrc1)
          == bitset_or_cmp (bdst, bsrc0, bsrc1));
  assert_bitset_equal (adst, bdst);

  /* xor */
  bitset_xor (adst, asrc0, asrc1);
  bitset_xor (bdst, bsrc0, bsrc1);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_xor_cmp (adst, asrc0, asrc1)
          == bitset_xor_cmp (bdst, bsrc0, bsrc1));
  assert_bitset_equal (adst, bdst);

  /* and_or */
  bitset_and_or (adst, asrc0, asrc1, asrc2);
  bitset_and_or (bdst, bsrc0, bsrc1, bsrc2);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_and_or_cmp (adst, asrc0, asrc1, asrc2)
          == bitset_and_or_cmp (bdst, bsrc0, bsrc1, bsrc2));
  assert_bitset_equal (adst, bdst);

  /* andn_or */
  bitset_andn_or (adst, asrc0, asrc1, asrc2);
  bitset_andn_or (bdst, bsrc0, bsrc1, bsrc2);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_andn_or_cmp (adst, asrc0, asrc1, asrc2)
          == bitset_andn_or_cmp (bdst, bsrc0, bsrc1, bsrc2));
  assert_bitset_equal (adst, bdst);

  /* or_and */
  bitset_or_and (adst, asrc0, asrc1, asrc2);
  bitset_or_and (bdst, bsrc0, bsrc1, bsrc2);
  assert_bitset_equal (adst, bdst);
  ASSERT (bitset_or_and_cmp (adst, asrc0, asrc1, asrc2)
          == bitset_or_and_cmp (bdst, bsrc0, bsrc1, bsrc2));
  assert_bitset_equal (adst, bdst);

  /* ones */
  bitset_ones (adst);
  bitset_ones (bdst);
  assert_bitset_equal (adst, bdst);

  /* zero */
  bitset_zero (adst);
  bitset_zero (bdst);
  assert_bitset_equal (adst, bdst);

  /* first and last and FOR_EACH.  */
  /* Work on bdst to exercise all the bitset types (adst is
     BITSET_VARIABLE).  */
  bitset_copy (bdst, bsrc0);
  debug_bitset (bdst);
  debug_bitset (bsrc0);
  bitset_copy (adst, bdst);

  /* count. */
  ASSERT (bitset_count (adst) == bitset_count (bdst));

  /* first and last */
  {
    bitset_bindex first = bitset_first (adst);
    ASSERT (first == bitset_first (bdst));

    bitset_bindex last  = bitset_last (adst);
    ASSERT (last == bitset_last (bdst));

    ASSERT (first <= last);
  }


  /* FOR_EACH.  */
  {
    bitset_iterator iter;
    bitset_bindex j;
    bitset_bindex first = bitset_first (bdst);
    bitset_bindex last  = bitset_last (bdst);
    bool seen_first = false;
    bool seen_last = false;
    BITSET_FOR_EACH (iter, bdst, j, 0)
      {
        ASSERT (first <= j && j <= last);
        ASSERT (bitset_test (bdst, j));
        if (j == first)
          seen_first = true;
        if (j == last)
          seen_last = true;
      }
    if (first == BITSET_BINDEX_MAX)
      {
        ASSERT (!seen_first);
        ASSERT (!seen_last);
      }
    else
      {
        ASSERT (seen_first);
        ASSERT (seen_last);
      }
  }

  /* FOR_EACH_REVERSE.  */
  {
    bitset_iterator iter;
    bitset_bindex j;
    bitset_bindex first = bitset_first (bdst);
    bitset_bindex last  = bitset_last (bdst);
    bool seen_first = false;
    bool seen_last = false;
    BITSET_FOR_EACH_REVERSE (iter, bdst, j, 0)
      {
        ASSERT (first <= j && j <= last);
        ASSERT (bitset_test (bdst, j));
        if (j == first)
          seen_first = true;
        if (j == last)
          seen_last = true;
      }
    if (first == BITSET_BINDEX_MAX)
      {
        ASSERT (!seen_first);
        ASSERT (!seen_last);
      }
    else
      {
        ASSERT (seen_first);
        ASSERT (seen_last);
      }
  }


  /* resize.

     ARRAY bitsets cannot be resized.  */
  if (bitset_type_get (bsrc0) != BITSET_ARRAY)
    {
      const int nbits_new = RANDOM (256);
      bitset_copy (adst, asrc0);
      bitset_copy (bdst, bsrc0);
      ASSERT (nbits_new == bitset_resize (adst, nbits_new));
      ASSERT (nbits_new == bitset_resize (bdst, nbits_new));
      assert_bitset_equal (adst, bdst);
    }

  bitset_free (bdst);
  bitset_free (bsrc3);
  bitset_free (bsrc2);
  bitset_free (bsrc1);
  bitset_free (bsrc0);
  bitset_free (adst);
  bitset_free (asrc3);
  bitset_free (asrc2);
  bitset_free (asrc1);
  bitset_free (asrc0);
}


/* Check empty bitsets.  */

static void
check_zero (bitset bs)
{
  fprintf (stderr, "check_zero\n");
  bitset_zero (bs);

  /* count. */
  ASSERT (bitset_count (bs) == 0);

  /* first and last */
  ASSERT (bitset_first (bs) == BITSET_BINDEX_MAX);
  ASSERT (bitset_last (bs)  == BITSET_BINDEX_MAX);

  /* FOR_EACH.  */
  {
    bitset_iterator iter;
    bitset_bindex i;
    BITSET_FOR_EACH (iter, bs, i, 0)
      ASSERT (0);
    BITSET_FOR_EACH_REVERSE (iter, bs, i, 0)
      ASSERT (0);
  }
}

/* Exercise on a single-bit values: it's easy to get the handling of
   the most significant bit wrong.  */

static void
check_one_bit (bitset bs, int bitno)
{
  fprintf (stderr, "check_one_bit(%d)\n", bitno);
  bitset_zero (bs);
  bitset_set (bs, bitno);

  /* count. */
  ASSERT (bitset_count (bs) == 1);

  /* test. */
  ASSERT (bitset_test (bs, bitno));

  /* first and last */
  ASSERT (bitset_first (bs) == bitno);
  ASSERT (bitset_last (bs) == bitno);

  /* FOR_EACH.  */
  {
    bitset_iterator iter;
    bitset_bindex i;
    BITSET_FOR_EACH (iter, bs, i, 0)
      ASSERT (i == bitno);

    BITSET_FOR_EACH_REVERSE (iter, bs, i, 0)
      ASSERT (i == bitno);
  }
}

/* Check full bitsets.  */

static void
check_ones (bitset bs)
{
  fprintf (stderr, "check_ones\n");
  const bitset_bindex size = bitset_size (bs);

  bitset_ones (bs);
  debug_bitset (bs);

  /* count. */
  ASSERT (bitset_count (bs) == size);

  /* first and last */
  ASSERT (bitset_first (bs) == 0);
  ASSERT (bitset_last (bs)  == size - 1);

  /* FOR_EACH.  */
  {
    bitset_iterator iter;
    bitset_bindex i;
    bitset_bindex count = 0;
    BITSET_FOR_EACH (iter, bs, i, 0)
      ASSERT (i == count++);
    ASSERT (count == size);
    BITSET_FOR_EACH_REVERSE (iter, bs, i, 0)
      ASSERT (i == --count);
    ASSERT (count == 0);
  }
}

/* Check various operations against expected values for a bitset
   having attributes ATTR.  */

static void
check_attributes (enum bitset_attr attr, int nbits)
{
  bitset bs0 = bitset_create (nbits, attr);
  ASSERT (bitset_size (bs0) == nbits);
  ASSERT (bitset_count (bs0) == 0);
  ASSERT (bitset_empty_p (bs0));

  bitset bs1 = bitset_create (nbits, attr);
  bitset_set (bs1, 1);
  bitset_set (bs1, 3);
  bitset_set (bs1, 5);
  ASSERT (bitset_count (bs1) == 3);
  ASSERT (!bitset_empty_p (bs1));

  bitset bs2 = bitset_create (nbits, attr);
  bitset_set (bs2, 0);
  bitset_set (bs2, 2);
  bitset_set (bs2, 4);

  /* disjoint_p */
  ASSERT (bitset_disjoint_p (bs1, bs2));

  /* and */
  bitset bs = bitset_create (nbits, attr);
  bitset_and (bs, bs1, bs2);
  ASSERT (bitset_count (bs) == 0);

  /* or */
  bitset_or (bs, bs1, bs2);
  ASSERT (bitset_count (bs) == 6);

  check_zero (bs);
  check_ones (bs);

  /* Exercise on all the single-bit values: it's easy to get the
     handling of the most significant bit wrong.  */
  for (int bitno = 0; bitno < nbits; ++bitno)
    check_one_bit (bs, bitno);

  bitset_free (bs);
  bitset_free (bs2);
  bitset_free (bs1);
  bitset_free (bs0);
}

int main (void)
{
  bitset_stats_enable ();

  for (int i = 0; i < 4; ++i)
    {
      /* table bitsets have elements that store 256 bits.  bitset_list
         (used in many operations) uses a cache whose size is
         BITSET_LIST_SIZE.  */
      int nbits =
        i == 0   ? 1
        : i == 1 ? 32
        : i == 2 ? 257
        :          (BITSET_LIST_SIZE + 1);
      check_attributes (BITSET_FIXED,    nbits);
      check_attributes (BITSET_VARIABLE, nbits);
      check_attributes (BITSET_DENSE,    nbits);
      check_attributes (BITSET_SPARSE,   nbits);
      check_attributes (BITSET_FRUGAL,   nbits);
      check_attributes (BITSET_GREEDY,   nbits);
    }

  compare (BITSET_VARIABLE, BITSET_FIXED);
  compare (BITSET_VARIABLE, BITSET_VARIABLE);
  compare (BITSET_VARIABLE, BITSET_DENSE);
  compare (BITSET_VARIABLE, BITSET_SPARSE);
  compare (BITSET_VARIABLE, BITSET_FRUGAL);
  compare (BITSET_VARIABLE, BITSET_GREEDY);

  bitset_stats_dump (stderr);
  return 0;
}
