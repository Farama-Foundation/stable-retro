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
   lists is weakly async-safe.

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

/* The number of different objects we can store in a bag.
   Must be a multiple of 32.  */
#  define BAG_SIZE 512

#  define RANDOM(n) (rand () % (n))
#  define RANDOM_OBJECT() ((void *) (uintptr_t) RANDOM (BAG_SIZE))

/* Representation of a bag as a bit set.  */
typedef struct { uint32_t bits[BAG_SIZE / 32]; } bag_t;

/* Initializes a bag to empty.  */
static void
init_bag_empty (bag_t *bagp)
{
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    bagp->bits[i] = 0;
}

/* Adds an element to a bag.  */
static void
add_to_bag (uintptr_t x, bag_t *bagp)
{
  if (!(x < BAG_SIZE))
    abort ();
  bagp->bits[x / 32] |= (1U << (x % 32));
}

/* Returns a bag that contains the elements of the given list.  */
static bag_t
bag_from_list (gl_list_t list)
{
  bag_t bag;

  init_bag_empty (&bag);
  {
    gl_list_iterator_t iter = gl_list_iterator (list);
    const void *elt;

    while (gl_list_iterator_next (&iter, &elt, NULL))
      add_to_bag ((uintptr_t) elt, &bag);
    gl_list_iterator_free (&iter);
  }

  return bag;
}

/* Returns true if and only if the given bag is empty.  */
static bool _GL_ATTRIBUTE_MAYBE_UNUSED
bag_is_empty (bag_t bag)
{
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    if (bag.bits[i] != 0)
      return false;
  return true;
}

/* Returns true if and only if BAG1 is a subset of BAG2.  */
static bool
bag_is_subset (bag_t bag1, bag_t bag2)
{
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    if ((bag1.bits[i] & ~bag2.bits[i]) != 0)
      return false;
  return true;
}

/* Returns true if and only if the two given bags have the same elements.  */
static bool
bag_equals (bag_t bag1, bag_t bag2)
{
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    if (bag1.bits[i] != bag2.bits[i])
      return false;
  return true;
}

/* Returns a bag that contains the elements of BAG1 and the elements of
   BAG2.  */
static bag_t _GL_ATTRIBUTE_MAYBE_UNUSED
bag_or (bag_t bag1, bag_t bag2)
{
  bag_t bag;
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    bag.bits[i] = bag1.bits[i] | bag2.bits[i];

  return bag;
}

/* Returns a bag that contains the elements of BAG1 that are not in BAG2
   and the elements of BAG2 that are not in BAG1.  */
static bag_t
bag_xor (bag_t bag1, bag_t bag2)
{
  bag_t bag;
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    bag.bits[i] = bag1.bits[i] ^ bag2.bits[i];

  return bag;
}

/* Returns a bag that contains the elements of BAG1 that are not in BAG2.  */
static bag_t _GL_ATTRIBUTE_MAYBE_UNUSED
bag_and_not (bag_t bag1, bag_t bag2)
{
  bag_t bag;
  size_t i;

  for (i = 0; i < BAG_SIZE / 32; i++)
    bag.bits[i] = bag1.bits[i] & ~bag2.bits[i];

  return bag;
}

/* test == 1: signals are executed in the mutator thread.
   test == 2: signals are executed in an idle thread.
   test == 3: signals are executed in the signal-sending thread.  */
static int volatile test;

/* Store the newest list's bag representation, the previous list's bag
   representation, and so on in a circular buffer.  Store also the
   changes in a circular buffer.  */
# define NUM_RECENT_BAGS 1024 /* can be up to (BAG_SIZE * BAG_SIZE) */
static bag_t volatile recent_bags[NUM_RECENT_BAGS];
static uintptr_t volatile recent_changes[NUM_RECENT_BAGS];
/* 0 <= recent_oldest_index <= recent_newest_index
   and recent_newest_index - recent_oldest_index < NUM_RECENT_BAGS.
   For each i with  recent_oldest_index <= i <= recent_newest_index, the bag
   representation of the list[i] is stored at recent_bags[i % NUM_RECENT_BAGS].
   For each i with  recent_oldest_index <= i < recent_newest_index, the object
   of the change between list[i] and list[i+1] is stored at
   recent_changes[i % NUM_RECENT_BAGS].  */
static size_t volatile recent_newest_index;
static size_t volatile recent_oldest_index;

static void
store_change (uintptr_t x, gl_list_t list)
{
  recent_oldest_index +=
    (recent_newest_index - recent_oldest_index) == NUM_RECENT_BAGS - 1;
  recent_changes[recent_newest_index % NUM_RECENT_BAGS] = x;
  recent_bags[(recent_newest_index + 1) % NUM_RECENT_BAGS] = bag_from_list (list);
  recent_newest_index++;
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

  bag_t bag;
  init_bag_empty (&bag);
  {
    gl_list_iterator_t iter = gl_list_iterator (list1);
    const void *elt;

    while (gl_list_iterator_next (&iter, &elt, NULL))
      add_to_bag ((uintptr_t) elt, &bag);
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
      bag_t changes_from_i_to_newest;
      init_bag_empty (&changes_from_i_to_newest);

      for (i = recent_newest_index;;)
        {
          if (bag_is_subset (bag_xor (bag, recent_bags[i % NUM_RECENT_BAGS]),
                             changes_from_i_to_newest)
              && i >= recent_oldest_index)
            {
              found = true;
              break;
            }
          if (i <= recent_oldest_index)
            break;
          i--;
          add_to_bag (recent_changes[i % NUM_RECENT_BAGS],
                      &changes_from_i_to_newest);
        }
    }
  else
    {
      /* The signal handler executes in the mutator thread.  Its execution
         interrupts a single mutation.  The allowed bag is either the newest
         or the previous one.  */
      size_t i = recent_newest_index;
      found = (bag_equals (bag, recent_bags[i % NUM_RECENT_BAGS])
               || (i > recent_oldest_index
                   && bag_equals (bag, recent_bags[(i - 1) % NUM_RECENT_BAGS])
                   && i > recent_oldest_index));
    }
  if (!found)
    {
      /* POSIX does not allow uses of stdio from within a signal handler, but
         it should work here, because the rest of the program does not use
         stdio.  */
      fprintf (stderr, "unexpected bag (after %u mutations and %u signals)\n",
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
            store_change ((uintptr_t) obj, list2);
            ASSERT (gl_list_nx_add_first (list1, obj) != NULL);
          }
          break;
        case 1: /* Remove an element.  */
          if (gl_list_size (list2) > 0)
            {
              size_t index = RANDOM (gl_list_size (list2));
              const void *obj = gl_list_get_at (list2, index);
              ASSERT (gl_list_remove (list2, obj));
              store_change ((uintptr_t) obj, list2);
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

    recent_oldest_index = 0;
    recent_bags[0] = bag_from_list (list2);
    recent_newest_index = 0;
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
