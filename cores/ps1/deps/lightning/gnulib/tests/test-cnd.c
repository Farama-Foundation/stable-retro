/* Test of condition variables in multithreaded situations.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

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

/* Which tests to perform.
   Uncomment some of these, to verify that all tests crash if no locking
   is enabled.  */
#define DO_TEST_COND 1
#define DO_TEST_TIMEDCOND 1


/* Whether to help the scheduler through explicit thrd_yield().
   Uncomment this to see if the operating system has a fair scheduler.  */
#define EXPLICIT_YIELD 1

/* Whether to print debugging messages.  */
#define ENABLE_DEBUGGING 0

#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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
# define yield() thrd_yield ()
#else
# define yield()
#endif


/*
 * Condition check
 */
static int cond_value = 0;
static cnd_t condtest;
static mtx_t lockcond;

static int
cnd_wait_routine (void *arg)
{
  ASSERT (mtx_lock (&lockcond) == thrd_success);
  while (!cond_value)
    {
      ASSERT (cnd_wait (&condtest, &lockcond) == thrd_success);
    }
  ASSERT (mtx_unlock (&lockcond) == thrd_success);

  cond_value = 2;

  return 0;
}

static void
test_cnd_wait ()
{
  struct timespec remain;
  thrd_t thread;
  int ret;

  remain.tv_sec = 2;
  remain.tv_nsec = 0;

  cond_value = 0;

  ASSERT (thrd_create (&thread, cnd_wait_routine, NULL) == thrd_success);
  do
    {
      yield ();
      ret = thrd_sleep (&remain, &remain);
      ASSERT (ret >= -1);
    }
  while (ret == -1 && (remain.tv_sec != 0 || remain.tv_nsec != 0));

  /* signal condition */
  ASSERT (mtx_lock (&lockcond) == thrd_success);
  cond_value = 1;
  ASSERT (cnd_signal (&condtest) == thrd_success);
  ASSERT (mtx_unlock (&lockcond) == thrd_success);

  ASSERT (thrd_join (thread, NULL) == thrd_success);

  if (cond_value != 2)
    abort ();
}


/*
 * Timed Condition check
 */
static int cond_timeout;

static void
get_ts (struct timespec *ts)
{
  struct timeval now;

  gettimeofday (&now, NULL);

  ts->tv_sec = now.tv_sec + 1;
  ts->tv_nsec = now.tv_usec * 1000;
}

static int
cnd_timedwait_routine (void *arg)
{
  int ret;
  struct timespec ts;

  ASSERT (mtx_lock (&lockcond) == thrd_success);
  while (!cond_value)
    {
      get_ts (&ts);
      ret = cnd_timedwait (&condtest, &lockcond, &ts);
      if (ret == thrd_timedout)
        cond_timeout = 1;
    }
  ASSERT (mtx_unlock (&lockcond) == thrd_success);

  return 0;
}

static void
test_cnd_timedwait (void)
{
  struct timespec remain;
  thrd_t thread;
  int ret;

  remain.tv_sec = 2;
  remain.tv_nsec = 0;

  cond_value = cond_timeout = 0;

  ASSERT (thrd_create (&thread, cnd_timedwait_routine, NULL) == thrd_success);
  do
    {
      yield ();
      ret = thrd_sleep (&remain, &remain);
      ASSERT (ret >= -1);
    }
  while (ret == -1 && (remain.tv_sec != 0 || remain.tv_nsec != 0));

  /* signal condition */
  ASSERT (mtx_lock (&lockcond) == thrd_success);
  cond_value = 1;
  ASSERT (cnd_signal (&condtest) == thrd_success);
  ASSERT (mtx_unlock (&lockcond) == thrd_success);

  ASSERT (thrd_join (thread, NULL) == thrd_success);

  if (!cond_timeout)
    abort ();
}

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

  ASSERT (cnd_init (&condtest) == thrd_success);
  ASSERT (mtx_init (&lockcond, mtx_plain) == thrd_success);

#if DO_TEST_COND
  printf ("Starting test_cnd_wait ..."); fflush (stdout);
  test_cnd_wait ();
  printf (" OK\n"); fflush (stdout);
#endif
#if DO_TEST_TIMEDCOND
  printf ("Starting test_cnd_timedwait ..."); fflush (stdout);
  test_cnd_timedwait ();
  printf (" OK\n"); fflush (stdout);
#endif

  return 0;
}
