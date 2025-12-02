/* Test of simple atomic operations for multithreading.
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

#include "simple-atomic.h"

#if USE_ISOC_THREADS || USE_POSIX_THREADS || USE_ISOC_AND_POSIX_THREADS || USE_WINDOWS_THREADS

/* Whether to help the scheduler through explicit yield().
   Uncomment this to see if the operating system has a fair scheduler.  */
#define EXPLICIT_YIELD 1

/* Number of simultaneous threads.  */
#define THREAD_COUNT 4

/* Number of operations performed in each thread.
   With a smaller count, say 100, we often get an "OK" result even with the racy
   implementation.  */
#define REPEAT_COUNT 1000

#include "glthread/thread.h"
#include "glthread/yield.h"

#include "macros.h"

#if EXPLICIT_YIELD
# define yield() gl_thread_yield ()
#else
# define yield()
#endif

/* Counters for each thread.  */
static unsigned int counter[THREAD_COUNT][5];

/* A variable of type 'unsigned int'.  */
static unsigned int int_variable;

static void *
int_mutator_thread (void *arg)
{
  int *pcounter = (int *) arg;
  int repeat;

  for (repeat = REPEAT_COUNT; repeat > 0; repeat--)
    {
      if (atomic_compare_and_swap (&int_variable, 0, 10) == 0)
        pcounter[0]++;
      yield ();

      if (atomic_compare_and_swap (&int_variable, 14, 17) == 14)
        pcounter[1]++;
      yield ();

      if (atomic_compare_and_swap (&int_variable, 20, 0) == 20)
        pcounter[2]++;
      yield ();

      if (atomic_compare_and_swap (&int_variable, 10, 14) == 10)
        pcounter[3]++;
      yield ();

      if (atomic_compare_and_swap (&int_variable, 17, 20) == 17)
        pcounter[4]++;
      yield ();
    }

  return NULL;
}

/* A variable of type 'uintptr_t'.  */
static uintptr_t ptr_variable;

static void *
ptr_mutator_thread (void *arg)
{
  int *pcounter = (int *) arg;
  int repeat;

  for (repeat = REPEAT_COUNT; repeat > 0; repeat--)
    {
      if (atomic_compare_and_swap_ptr (&ptr_variable, 0, 10) == 0)
        pcounter[0]++;
      yield ();

      if (atomic_compare_and_swap_ptr (&ptr_variable, 14, 17) == 14)
        pcounter[1]++;
      yield ();

      if (atomic_compare_and_swap_ptr (&ptr_variable, 20, 0) == 20)
        pcounter[2]++;
      yield ();

      if (atomic_compare_and_swap_ptr (&ptr_variable, 10, 14) == 10)
        pcounter[3]++;
      yield ();

      if (atomic_compare_and_swap_ptr (&ptr_variable, 17, 20) == 17)
        pcounter[4]++;
      yield ();
    }

  return NULL;
}

int
main ()
{
  /* Check simple uses of atomic_compare_and_swap.  */
  {
    unsigned int x[3] = { 0xDEADBEEFU, 11, 0xDEADBEEFU };

    ASSERT (atomic_compare_and_swap (&x[1], 0, 17) == 11);
    ASSERT (x[1] == 11);

    ASSERT (atomic_compare_and_swap (&x[1], 4, 11) == 11);
    ASSERT (x[1] == 11);

    ASSERT (atomic_compare_and_swap (&x[1], 11, 15) == 11);
    ASSERT (x[1] == 15);

    ASSERT (x[0] == 0xDEADBEEFU);
    ASSERT (x[2] == 0xDEADBEEFU);
  }

  /* Check simple uses of atomic_compare_and_swap_ptr.  */
  {
    uintptr_t v1 = ~(uintptr_t)0 / 3;
    uintptr_t v2 = ~(uintptr_t)0 / 5 * 4;
    uintptr_t v3 = ~(uintptr_t)0 / 7 * 3;
    uintptr_t x[3] = { 0xDEADBEEFU, v1, 0xDEADBEEFU };

    ASSERT (atomic_compare_and_swap_ptr (&x[1], 0, v3) == v1);
    ASSERT (x[1] == v1);

    ASSERT (atomic_compare_and_swap_ptr (&x[1], 4, v1) == v1);
    ASSERT (x[1] == v1);

    ASSERT (atomic_compare_and_swap_ptr (&x[1], v1, v2) == v1);
    ASSERT (x[1] == v2);

    ASSERT (x[0] == 0xDEADBEEFU);
    ASSERT (x[2] == 0xDEADBEEFU);
  }

  /* Check atomicity of atomic_compare_and_swap.  */
  {
    void * (*funcs[2]) (void *) = { int_mutator_thread, ptr_mutator_thread };
    int f;
    for (f = 0; f < 2; f++)
      {
        void * (*func) (void *) = funcs[f];
        int i, j;
        gl_thread_t threads[THREAD_COUNT];

        /* Initialization.  */
        for (i = 0; i < THREAD_COUNT; i++)
          for (j = 0; j < 5; j++)
            counter[i][j] = 0;

        /* Spawn the threads.  */
        for (i = 0; i < THREAD_COUNT; i++)
          threads[i] = gl_thread_create (func, &counter[i][0]);

        /* Wait for the threads to terminate.  */
        for (i = 0; i < THREAD_COUNT; i++)
          gl_thread_join (threads[i], NULL);

        /* Sum up the work that the threads have done.  */
        unsigned int sum[5];
        for (j = 0; j < 5; j++)
          {
            sum[j] = 0;
            for (i = 0; i < THREAD_COUNT; i++)
              sum[j] += counter[i][j];
          }

        /* If things went atomically, the threads have moved the variable's
           value through the cycle  0 -> 10 -> 14 -> 17 -> 20 -> 0 ...  a large
           number of times.
           sum[0] is the number of transitions 0 -> 10.
           sum[3] is the number of transitions 10 -> 14.
           sum[1] is the number of transitions 14 -> 17.
           sum[4] is the number of transitions 17 -> 20.
           sum[2] is the number of transitions 20 -> 0.
           Since the cycle started at 0 and ends anywhere (namely, when all
           threads when through their loop REPEAT_COUNT times), the sequence
             sum[0], sum[3], sum[1], sum[4], sum[2], sum[0] - 1
           must be monotonically decreasing.

           If things did not go atomically, the counters don't exhibit this
           pattern.  */
        printf ("Counters: %u %u %u %u %u\n",
                sum[0], sum[3], sum[1], sum[4], sum[2]);
        ASSERT ((sum[0] == sum[3] || sum[0] == sum[3] + 1)
                && (sum[3] == sum[1] || sum[3] == sum[1] + 1)
                && (sum[1] == sum[4] || sum[1] == sum[4] + 1)
                && (sum[4] == sum[2] || sum[4] == sum[2] + 1)
                && (sum[2] + 1 == sum[0] || sum[2] == sum[0]));
      }
  }

  return 0;
}

#else

/* No multithreading available.  */

#include <stdio.h>

int
main ()
{
  fputs ("Skipping test: multithreading not enabled\n", stderr);
  return 77;
}

#endif
