/* Test of thread-specific storage in multithreaded situations.
   Copyright (C) 2005, 2008-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2005.  */

#include <config.h>

#if USE_ISOC_THREADS || USE_POSIX_THREADS || USE_ISOC_AND_POSIX_THREADS || USE_WINDOWS_THREADS

/* Whether to help the scheduler through explicit sched_yield().
   Uncomment this to see if the operating system has a fair scheduler.  */
#define EXPLICIT_YIELD 1

/* Whether to print debugging messages.  */
#define ENABLE_DEBUGGING 0

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if EXPLICIT_YIELD
# include <sched.h>
#endif

#if HAVE_DECL_ALARM
# include <signal.h>
# include <unistd.h>
#endif

#include "macros.h"

#if ENABLE_DEBUGGING
# define dbgprintf printf
#else
# define dbgprintf if (0) printf
#endif

#if EXPLICIT_YIELD
# define yield() sched_yield ()
#else
# define yield()
#endif

/* Returns a reference to the current thread as a pointer, for debugging.  */
#if defined __MVS__
  /* On IBM z/OS, pthread_t is a struct with an 8-byte '__' field.
     The first three bytes of this field appear to uniquely identify a
     pthread_t, though not necessarily representing a pointer.  */
# define pthread_self_pointer() (*((void **) pthread_self ().__))
#else
# define pthread_self_pointer() ((void *) (uintptr_t) pthread_self ())
#endif

static void
perhaps_yield (void)
{
  /* Call yield () only with a certain probability, otherwise the
     sequence of thread activations may be too predictable.  */
  if ((((unsigned int) rand () >> 3) % 4) == 0)
    yield ();
}


/* ----------------------- Test thread-local storage ----------------------- */

/* Number of simultaneous threads.  */
#define THREAD_COUNT 16

/* Number of operations performed in each thread.  */
#define REPEAT_COUNT 50000

#define KEYS_COUNT 4

static pthread_key_t mykeys[KEYS_COUNT];

static void *
worker_thread (void *arg)
{
  unsigned int id = (unsigned int) (uintptr_t) arg;
  int i, j, repeat;
  unsigned int values[KEYS_COUNT];

  dbgprintf ("Worker %p started\n", pthread_self_pointer ());

  /* Initialize the per-thread storage.  */
  for (i = 0; i < KEYS_COUNT; i++)
    {
      values[i] = (((unsigned int) rand () >> 3) % 1000000) * THREAD_COUNT + id;
      /* Hopefully no arithmetic overflow.  */
      if ((values[i] % THREAD_COUNT) != id)
        abort ();
    }
  perhaps_yield ();

  /* Verify that the initial value is NULL.  */
  dbgprintf ("Worker %p before initial verify\n", pthread_self_pointer ());
  for (i = 0; i < KEYS_COUNT; i++)
    if (pthread_getspecific (mykeys[i]) != NULL)
      abort ();
  dbgprintf ("Worker %p after  initial verify\n", pthread_self_pointer ());
  perhaps_yield ();

  /* Initialize the per-thread storage.  */
  dbgprintf ("Worker %p before first pthread_setspecific\n",
             pthread_self_pointer ());
  for (i = 0; i < KEYS_COUNT; i++)
    {
      unsigned int *ptr = (unsigned int *) malloc (sizeof (unsigned int));
      *ptr = values[i];
      ASSERT (pthread_setspecific (mykeys[i], ptr) == 0);
    }
  dbgprintf ("Worker %p after  first pthread_setspecific\n",
             pthread_self_pointer ());
  perhaps_yield ();

  /* Shuffle around the pointers.  */
  for (repeat = REPEAT_COUNT; repeat > 0; repeat--)
    {
      dbgprintf ("Worker %p doing value swapping\n", pthread_self_pointer ());
      i = ((unsigned int) rand () >> 3) % KEYS_COUNT;
      j = ((unsigned int) rand () >> 3) % KEYS_COUNT;
      if (i != j)
        {
          void *vi = pthread_getspecific (mykeys[i]);
          void *vj = pthread_getspecific (mykeys[j]);

          ASSERT (pthread_setspecific (mykeys[i], vj) == 0);
          ASSERT (pthread_setspecific (mykeys[j], vi) == 0);
        }
      perhaps_yield ();
    }

  /* Verify that all the values are from this thread.  */
  dbgprintf ("Worker %p before final verify\n", pthread_self_pointer ());
  for (i = 0; i < KEYS_COUNT; i++)
    if ((*(unsigned int *) pthread_getspecific (mykeys[i]) % THREAD_COUNT)
        != id)
      abort ();
  dbgprintf ("Worker %p after  final verify\n", pthread_self_pointer ());
  perhaps_yield ();

  dbgprintf ("Worker %p dying.\n", pthread_self_pointer ());
  return NULL;
}

static void
test_tss (void)
{
  int pass, i;

  for (pass = 0; pass < 2; pass++)
    {
      pthread_t threads[THREAD_COUNT];

      if (pass == 0)
        for (i = 0; i < KEYS_COUNT; i++)
          ASSERT (pthread_key_create (&mykeys[i], free) == 0);
      else
        for (i = KEYS_COUNT - 1; i >= 0; i--)
          ASSERT (pthread_key_create (&mykeys[i], free) == 0);

      /* Spawn the threads.  */
      for (i = 0; i < THREAD_COUNT; i++)
        ASSERT (pthread_create (&threads[i], NULL,
                                worker_thread, (void *) (uintptr_t) i)
                == 0);

      /* Wait for the threads to terminate.  */
      for (i = 0; i < THREAD_COUNT; i++)
        ASSERT (pthread_join (threads[i], NULL) == 0);

      for (i = 0; i < KEYS_COUNT; i++)
        ASSERT (pthread_key_delete (mykeys[i]) == 0);
    }
}

#undef KEYS_COUNT
#undef REPEAT_COUNT
#undef THREAD_COUNT


/* --------------- Test thread-local storage with destructors --------------- */

/* Number of simultaneous threads.  */
#if defined __ANDROID__
# define THREAD_COUNT 5   /* to avoid a pthread_key_create failure */
#else
# define THREAD_COUNT 10
#endif

/* Number of keys to allocate in each thread.  */
#define KEYS_COUNT 10

static pthread_mutex_t sumlock;
static uintptr_t sum;

static void
inc_sum (uintptr_t value)
{
  ASSERT (pthread_mutex_lock (&sumlock) == 0);
  sum += value;
  ASSERT (pthread_mutex_unlock (&sumlock) == 0);
}

static void
destructor0 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 0)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor1 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 1)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor2 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 2)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor3 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 3)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor4 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 4)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor5 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 5)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor6 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 6)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor7 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 7)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor8 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 8)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void
destructor9 (void *value)
{
  if ((((uintptr_t) value - 1) % 10) != 9)
    abort ();
  inc_sum ((uintptr_t) value);
}

static void (*destructor_table[10]) (void *) =
  {
    destructor0,
    destructor1,
    destructor2,
    destructor3,
    destructor4,
    destructor5,
    destructor6,
    destructor7,
    destructor8,
    destructor9
  };

static pthread_key_t dtorcheck_keys[THREAD_COUNT][KEYS_COUNT];

/* Worker thread that uses destructors that verify that the destructor belongs
   to the right thread.  */
static void *
dtorcheck1_thread (void *arg)
{
  unsigned int id = (unsigned int) (uintptr_t) arg;
  pthread_key_t *keys = dtorcheck_keys[id]; /* an array of KEYS_COUNT keys */
  int i;

  for (i = 0; i < KEYS_COUNT; i++)
    ASSERT (pthread_key_create (&keys[i], destructor_table[i]) == 0);

  for (i = 0; i < KEYS_COUNT; i++)
    ASSERT (pthread_setspecific (keys[i],
                                 (void *) (uintptr_t) (10 * id + i + 1))
            == 0);

  return NULL;
}

static void
test_tss_dtorcheck1 (void)
{
  pthread_t threads[THREAD_COUNT];
  unsigned int id;
  int i;
  uintptr_t expected_sum;

  sum = 0;

  /* Spawn the threads.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_create (&threads[id], NULL,
                            dtorcheck1_thread, (void *) (uintptr_t) id)
            == 0);

  /* Wait for the threads to terminate.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_join (threads[id], NULL) == 0);

  /* Clean up the keys.  */
  for (id = 0; id < THREAD_COUNT; id++)
    for (i = 0; i < KEYS_COUNT; i++)
      ASSERT (pthread_key_delete (dtorcheck_keys[id][i]) == 0);

  /* Check that the destructor was invoked for each key.  */
  expected_sum = 10 * KEYS_COUNT * (THREAD_COUNT * (THREAD_COUNT - 1) / 2)
                 + THREAD_COUNT * (KEYS_COUNT * (KEYS_COUNT - 1) / 2)
                 + THREAD_COUNT * KEYS_COUNT;
  if (sum != expected_sum)
    abort ();
}

/* Worker thread that uses destructors that verify that the destructor belongs
   to the right key allocated within the thread.  */
static void *
dtorcheck2_thread (void *arg)
{
  unsigned int id = (unsigned int) (uintptr_t) arg;
  pthread_key_t *keys = dtorcheck_keys[id]; /* an array of KEYS_COUNT keys */
  int i;

  for (i = 0; i < KEYS_COUNT; i++)
    ASSERT (pthread_key_create (&keys[i], destructor_table[id]) == 0);

  for (i = 0; i < KEYS_COUNT; i++)
    ASSERT (pthread_setspecific (keys[i],
                                 (void *) (uintptr_t) (10 * i + id + 1))
            == 0);

  return NULL;
}

static void
test_tss_dtorcheck2 (void)
{
  pthread_t threads[THREAD_COUNT];
  unsigned int id;
  int i;
  uintptr_t expected_sum;

  sum = 0;

  /* Spawn the threads.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_create (&threads[id], NULL,
                            dtorcheck2_thread, (void *) (uintptr_t) id)
            == 0);

  /* Wait for the threads to terminate.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_join (threads[id], NULL) == 0);

  /* Clean up the keys.  */
  for (id = 0; id < THREAD_COUNT; id++)
    for (i = 0; i < KEYS_COUNT; i++)
      ASSERT (pthread_key_delete (dtorcheck_keys[id][i]) == 0);

  /* Check that the destructor was invoked for each key.  */
  expected_sum = 10 * THREAD_COUNT * (KEYS_COUNT * (KEYS_COUNT - 1) / 2)
                 + KEYS_COUNT * (THREAD_COUNT * (THREAD_COUNT - 1) / 2)
                 + THREAD_COUNT * KEYS_COUNT;
  if (sum != expected_sum)
    abort ();
}

#undef KEYS_COUNT
#undef THREAD_COUNT


/* --- Test thread-local storage with with races between init and destroy --- */

/* Number of simultaneous threads.  */
#if defined __ANDROID__
# define THREAD_COUNT 5   /* to avoid a pthread_key_create failure */
#else
# define THREAD_COUNT 10
#endif

/* Number of keys to allocate in each thread.  */
#define KEYS_COUNT 10

/* Number of times to destroy and reallocate a key in each thread.  */
#define REPEAT_COUNT 100000

static pthread_key_t racecheck_keys[THREAD_COUNT][KEYS_COUNT];

/* Worker thread that does many destructions and reallocations of keys, and also
   uses destructors that verify that the destructor belongs to the right key.  */
static void *
racecheck_thread (void *arg)
{
  unsigned int id = (unsigned int) (uintptr_t) arg;
  pthread_key_t *keys = racecheck_keys[id]; /* an array of KEYS_COUNT keys */
  int repeat;
  int i;

  dbgprintf ("Worker %p started\n", pthread_self_pointer ());

  for (i = 0; i < KEYS_COUNT; i++)
    {
      ASSERT (pthread_key_create (&keys[i], destructor_table[i]) == 0);
      ASSERT (pthread_setspecific (keys[i],
                                   (void *) (uintptr_t) (10 * id + i + 1))
              == 0);
    }

  for (repeat = REPEAT_COUNT; repeat > 0; repeat--)
    {
      i = ((unsigned int) rand () >> 3) % KEYS_COUNT;
      dbgprintf ("Worker %p reallocating key %d\n", pthread_self_pointer (), i);
      ASSERT (pthread_key_delete (keys[i]) == 0);
      ASSERT (pthread_key_create (&keys[i], destructor_table[i]) == 0);
      ASSERT (pthread_setspecific (keys[i],
                                   (void *) (uintptr_t) (10 * id + i + 1))
              == 0);
    }

  dbgprintf ("Worker %p dying.\n", pthread_self_pointer ());
  return NULL;
}

static void
test_tss_racecheck (void)
{
  pthread_t threads[THREAD_COUNT];
  unsigned int id;
  int i;
  uintptr_t expected_sum;

  sum = 0;

  /* Spawn the threads.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_create (&threads[id], NULL,
                            racecheck_thread, (void *) (uintptr_t) id)
            == 0);

  /* Wait for the threads to terminate.  */
  for (id = 0; id < THREAD_COUNT; id++)
    ASSERT (pthread_join (threads[id], NULL) == 0);

  /* Clean up the keys.  */
  for (id = 0; id < THREAD_COUNT; id++)
    for (i = 0; i < KEYS_COUNT; i++)
      ASSERT (pthread_key_delete (racecheck_keys[id][i]) == 0);

  /* Check that the destructor was invoked for each key.  */
  expected_sum = 10 * KEYS_COUNT * (THREAD_COUNT * (THREAD_COUNT - 1) / 2)
                 + THREAD_COUNT * (KEYS_COUNT * (KEYS_COUNT - 1) / 2)
                 + THREAD_COUNT * KEYS_COUNT;
  if (sum != expected_sum)
    abort ();
}

#undef REPEAT_COUNT
#undef KEYS_COUNT
#undef THREAD_COUNT


/* -------------------------------------------------------------------------- */

int
main ()
{
#if HAVE_DECL_ALARM
  /* Declare failure if test takes too long, by using default abort
     caused by SIGALRM.  */
  int alarm_value = 600;
  signal (SIGALRM, SIG_DFL);
  alarm (alarm_value);
#endif

  {
    pthread_mutexattr_t attr;

    ASSERT (pthread_mutexattr_init (&attr) == 0);
    ASSERT (pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_NORMAL) == 0);
    ASSERT (pthread_mutex_init (&sumlock, &attr) == 0);
    ASSERT (pthread_mutexattr_destroy (&attr) == 0);
  }

  printf ("Starting test_tss ..."); fflush (stdout);
  test_tss ();
  printf (" OK\n"); fflush (stdout);

  printf ("Starting test_tss_dtorcheck1 ..."); fflush (stdout);
  test_tss_dtorcheck1 ();
  printf (" OK\n"); fflush (stdout);

  printf ("Starting test_tss_dtorcheck2 ..."); fflush (stdout);
  test_tss_dtorcheck2 ();
  printf (" OK\n"); fflush (stdout);

  printf ("Starting test_tss_racecheck ..."); fflush (stdout);
  test_tss_racecheck ();
  printf (" OK\n"); fflush (stdout);

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
