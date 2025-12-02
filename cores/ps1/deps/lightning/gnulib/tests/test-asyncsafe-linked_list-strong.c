/* Test of async-safe sequential list data type implementation.
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

/* There are two notions of async-safe for a list:
     * A list is "strongly async-safe" if it can be iterated in any signal
       handler, and the signal handler will see
         - in the single-threaded case: either the value after or the value
           before the current in-progress change that was interrupted,
         - in the multi-threaded case: one of the most recent values for the
           *entire list*.
     * A list is "weakly async-safe" if it can be iterated in any signal
       handler, and
         - in the single-threaded case: the signal handler will see either
           the value after or the value before the current in-progress change
           that was interrupted,
         - in the multi-threaded case:
           - elements which were present in the list throughout the iteration
             will be seen by the iterator,
           - elements which were absent in the list throughout the iteration
             will be unseen by the iterator,
           - no statement regarding the elements which are being added or
             removed during the iteration.

   This unit test attempts to verify that the 'linked-list' implementation of
   lists is strongly async-safe.

   It fails the test in the multi-threaded cases (test == 2 or 3).  */

#include <config.h>

/* Work around GCC bug 44511.  */
#if 4 < __GNUC__ + (3 <= __GNUC_MINOR__)
# pragma GCC diagnostic ignored "-Wreturn-type"
#endif

#include "gl_linked_list.h"

#if SIGNAL_SAFE_LIST

# if USE_ISOC_THREADS || USE_POSIX_THREADS || USE_ISOC_AND_POSIX_THREADS /* || USE_WINDOWS_THREADS */
/* This test works only with POSIX compatible threads.
   With Windows threads, send_signal() has to be implemented as
     raise (MY_SIGNAL);
   hence only test == 3 tests anything, and in fact only 5 signals arrive.
   This small test is not able to detect a buggy implementation.  Therefore
   mark the test as skipped in this case.  */

#  include <signal.h>
#  include <stdint.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <time.h>

#  include "glthread/thread.h"
#  include "glthread/yield.h"

#  include "macros.h"

/* The signal we use for testing.
   For portability, it should be one of SIGINT, SIGFPE, SIGTERM.
   If we use SIGINT, it prevents debugging with gdb.
   If we use SIGFPE, it does not work right with valgrind.
   If we use SIGTERM, it could interfere with a system shutdown.
   Oh well.  */
#  define MY_SIGNAL SIGTERM

#  define RANDOM(n) (rand () % (n))
#  define RANDOM_OBJECT() ((void *) (uintptr_t) RANDOM (10000))

/* test == 1: signals are executed in the mutator thread.
   test == 2: signals are executed in an idle thread.
   test == 3: signals are executed in the signal-sending thread.  */
static int volatile test;

/* Store the newest sum, the previous sum, the sum before the previous sum,
   and so on in a circular buffer.  */
#  define NUM_RECENT_SUMS (4*1024*1024)
static uintptr_t volatile recent_sums[NUM_RECENT_SUMS];
/* 0 <= recent_sums_oldest_index < recent_sums_newest_index
   and recent_sums_newest_index - recent_sums_oldest_index <= NUM_RECENT_SUMS.
   For each i with  recent_sums_oldest_index <= i < recent_sums_newest_index,
   the sum is actually stored at recent_sums[i % NUM_RECENT_SUMS].  */
static size_t volatile recent_sums_newest_index;
static size_t volatile recent_sums_oldest_index;

static void
store_newest_sum (uintptr_t sum)
{
  recent_sums_oldest_index +=
    (recent_sums_newest_index - recent_sums_oldest_index) == NUM_RECENT_SUMS;
  recent_sums[recent_sums_newest_index % NUM_RECENT_SUMS] = sum;
  recent_sums_newest_index++;
}

static gl_list_t volatile list1;

static gl_thread_t volatile signal_target;

/* Statistics.  */
static unsigned int volatile num_mutations;
static unsigned int volatile num_signals_sent;
static unsigned int volatile num_signals_arrived;

/* Blocks the MY_SIGNAL signal in the current thread.  */
static void
block_sigint (void)
{
  sigset_t mask;

  sigemptyset (&mask);
  sigaddset (&mask, MY_SIGNAL);
  sigprocmask (SIG_BLOCK, &mask, NULL); /* equivalent to pthread_sigmask */
}

/* This thread is idle.  */
static void *
idle_thread (void *arg)
{
  for (;;)
    gl_thread_yield ();

  /*NOTREACHED*/
}

/* The signal handler iterates through the list and verifies that the sum of
   all elements in the list is one of the allowed values.  */
static _GL_ASYNC_SAFE void
sigint_handler (int signo)
{
  num_signals_arrived++;

  uintptr_t sum = 0;
  {
    gl_list_iterator_t iter = gl_list_iterator (list1);
    const void *elt;

    while (gl_list_iterator_next (&iter, &elt, NULL))
      sum += (uintptr_t) elt;
    gl_list_iterator_free (&iter);
  }

  bool found = false;
  if (test != 1)
    {
      /* The signal handler executes in a different thread than the mutator
         thread.  By the time we get here, the mutator thread can have done
         any number of mutations; it is reasonable to assume that this number
         of mutations is small.  */
      size_t i;
      for (i = recent_sums_newest_index - 1;;)
        {
          if (sum == recent_sums[i % NUM_RECENT_SUMS]
              && i >= recent_sums_oldest_index)
            {
              found = true;
              break;
            }
          if (i <= recent_sums_oldest_index)
            break;
          i--;
        }
    }
  else
    {
      /* The signal handler executes in the mutator thread.  Its execution
         interrupts a single mutation.  The allowed sum is either the newest
         or the previous one.  */
      size_t i = recent_sums_newest_index - 1;
      found = (sum == recent_sums[i % NUM_RECENT_SUMS]
               || (i > recent_sums_oldest_index
                   && sum == recent_sums[(i - 1) % NUM_RECENT_SUMS]));
    }
  if (!found)
    {
      /* POSIX does not allow uses of stdio from within a signal handler, but
         it should work here, because the rest of the program does not use
         stdio.  */
      size_t i = recent_sums_newest_index - 1;
      fprintf (stderr, "sum = %lu, expected %lu or older (after %u mutations and %u signals)\n",
               (unsigned long) sum,
               (unsigned long) recent_sums[i % NUM_RECENT_SUMS],
               num_mutations, num_signals_arrived);
      fflush (stderr);
      abort ();
    }
}

/* Sends a MY_SIGNAL signal to the current process.  */
static void
send_signal (void)
{
#if 0
  /* This does not work for test != 3, because raise() sends the signal to the
     current thread always, and if test != 3 the signal is eternally blocked
     in the current thread; thus it will never get delivered.  */
  raise (MY_SIGNAL);
#else
  /* This works: Either
       kill (getpid (), MY_SIGNAL);
     or
       pthread_kill (signal_target, MY_SIGNAL);
     The difference is that for kill(), the OS determines the (only) thread that
     has not blocked the signal, whereas for pthread_kill() we have manually
     determined this thread.  */
  kill (getpid (), MY_SIGNAL);
#endif
}

/* This thread permanently sends MY_SIGNAL signals.  */
static void *
signal_sending_thread (void *arg)
{
  if (test != 3)
    block_sigint ();

  int repeat;

  for (repeat = 1000; repeat > 0; repeat--)
    {
      num_signals_sent++; send_signal ();
      num_signals_sent++; send_signal ();
      num_signals_sent++; send_signal ();
      num_signals_sent++; send_signal ();
      num_signals_sent++; send_signal ();
      {
        struct timespec ts;
        ts.tv_sec = 0; ts.tv_nsec = 1000000;
        nanosleep (&ts, NULL);
      }
      gl_thread_yield ();
    }

  printf ("Sent %u signals. Received %u signals. Done after %u mutations.\n",
          num_signals_sent, num_signals_arrived, num_mutations);

  exit (0);

  /*NOTREACHED*/
}

/* This thread repeatedly adds and removes objects from the list.  */
static void *
mutator_thread (void *arg)
{
  if (test != 1)
    block_sigint ();

  gl_list_t list2 = (gl_list_t) arg;

  for (num_mutations = 0; ; num_mutations++)
    {
      unsigned int operation = RANDOM (2);
      switch (operation)
        {
        case 0: /* Add an element.  */
          {
            const void *obj = RANDOM_OBJECT ();
            ASSERT (gl_list_nx_add_first (list2, obj) != NULL);
            uintptr_t sum_before_current_operation =
              recent_sums[(recent_sums_newest_index - 1) % NUM_RECENT_SUMS];
            uintptr_t sum_after_current_operation =
              sum_before_current_operation + (uintptr_t) obj;
            store_newest_sum (sum_after_current_operation);
            ASSERT (gl_list_nx_add_first (list1, obj) != NULL);
          }
          break;
        case 1: /* Remove an element.  */
          if (gl_list_size (list2) > 0)
            {
              size_t index = RANDOM (gl_list_size (list2));
              const void *obj = gl_list_get_at (list2, index);
              ASSERT (gl_list_remove (list2, obj));
              uintptr_t sum_before_current_operation =
                recent_sums[(recent_sums_newest_index - 1) % NUM_RECENT_SUMS];
              uintptr_t sum_after_current_operation =
                sum_before_current_operation - (uintptr_t) obj;
              store_newest_sum (sum_after_current_operation);
              ASSERT (gl_list_remove (list1, obj));
            }
          break;
        }
    }

  /*NOTREACHED*/
}

int
main (int argc, char *argv[])
{
  test = atoi (argv[1]);

  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 2)
    srand (atoi (argv[2]));

  gl_list_t list2;
  /* Initialize list1 and list2.  */
  {
    size_t initial_size = RANDOM (50);
    const void **contents =
      (const void **) malloc (initial_size * sizeof (const void *));
    size_t i;

    for (i = 0; i < initial_size; i++)
      contents[i] = RANDOM_OBJECT ();

    list1 = gl_list_nx_create_empty (GL_LINKED_LIST, NULL, NULL, NULL, true);
    ASSERT (list1 != NULL);
    for (i = 0; i < initial_size; i++)
      ASSERT (gl_list_nx_add_first (list1, contents[i]) != NULL);

    list2 = gl_list_nx_create_empty (GL_LINKED_LIST, NULL, NULL, NULL, true);
    ASSERT (list2 != NULL);
    for (i = 0; i < initial_size; i++)
      ASSERT (gl_list_nx_add_last (list2, contents[i]) != NULL);

    uintptr_t initial_sum = 0;
    for (i = 0; i < initial_size; i++)
      initial_sum += (uintptr_t) contents[i];
    recent_sums_oldest_index = 0;
    recent_sums[0] = initial_sum;
    recent_sums_newest_index = 1;
  }

  /* Install the signal handler.
     It needs to be done with sigaction(), not signal(), otherwise on Solaris
     and AIX the program crashes at the second incoming MY_SIGNAL.  */
  #if 0
  signal (MY_SIGNAL, sigint_handler);
  #else
  {
    struct sigaction action;
    action.sa_handler = sigint_handler;
    action.sa_flags = SA_RESTART | SA_NODEFER;
    sigemptyset (&action.sa_mask);
    ASSERT (sigaction (MY_SIGNAL, &action, NULL) == 0);
  }
  #endif

  /* Launch the threads.  */
  switch (test)
    {
    case 1:
      {
        signal_target = gl_thread_create (mutator_thread, list2);
        signal_sending_thread (NULL);
        abort ();
      }

    case 2:
      {
        signal_target = gl_thread_create (idle_thread, NULL);
        gl_thread_create (mutator_thread, list2);
        signal_sending_thread (NULL);
        abort ();
      }

    case 3:
      {
        gl_thread_create (mutator_thread, list2);
        signal_target = gl_thread_self (); signal_sending_thread (NULL);
        abort ();
      }

    default:
      ASSERT (false);
    }
}

# else

#  include <stdio.h>

int
main ()
{
  fputs ("Skipping test: POSIX compatible multithreading not enabled\n", stderr);
  return 77;
}

# endif

#else

# include <stdio.h>

int
main ()
{
  fputs ("Skipping test: signal-safety of linked lists is not enabled\n", stderr);
  return 77;
}

#endif
