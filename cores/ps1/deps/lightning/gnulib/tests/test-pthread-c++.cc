/* Test of <pthread.h> substitute in C++ mode.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2019.  */

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#include <pthread.h>

#include "signature.h"


/* =========== Thread functions =========== */

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_create, int,
                 (pthread_t *, const pthread_attr_t *,
                  void * (*) (void*), void *));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_attr_init, int, (pthread_attr_t *));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_attr_getdetachstate, int,
                 (const pthread_attr_t *, int *));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_attr_setdetachstate, int,
                 (pthread_attr_t *, int));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_attr_destroy, int,
                 (pthread_attr_t *));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_self, pthread_t, (void));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_equal, int, (pthread_t, pthread_t));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_detach, int, (pthread_t));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_join, int, (pthread_t, void **));
#endif

#if GNULIB_TEST_PTHREAD_THREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_exit, void, (void *));
#endif

/* =========== Once-only control (initialization) functions =========== */

#if GNULIB_TEST_PTHREAD_ONCE
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_once, int,
                 (pthread_once_t *, void (*) (void)));
#endif

/* =========== Mutex functions =========== */

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_init, int,
                 (pthread_mutex_t *, const pthread_mutexattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_init, int,
                 (pthread_mutexattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_gettype, int,
                 (const pthread_mutexattr_t *, int *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_settype, int,
                 (pthread_mutexattr_t *, int));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_getrobust, int,
                 (const pthread_mutexattr_t *, int *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_setrobust, int,
                 (pthread_mutexattr_t *, int));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutexattr_destroy, int,
                 (pthread_mutexattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_lock, int,
                 (pthread_mutex_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_trylock, int,
                 (pthread_mutex_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX_TIMEDLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_timedlock, int,
                 (pthread_mutex_t *, const struct timespec *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_unlock, int,
                 (pthread_mutex_t *));
#endif

#if GNULIB_TEST_PTHREAD_MUTEX
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_mutex_destroy, int,
                 (pthread_mutex_t *));
#endif

/* =========== Read-write lock functions =========== */

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_init, int,
                 (pthread_rwlock_t *, const pthread_rwlockattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlockattr_init, int,
                 (pthread_rwlockattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlockattr_destroy, int,
                 (pthread_rwlockattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_rdlock, int,
                 (pthread_rwlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_wrlock, int,
                 (pthread_rwlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_tryrdlock, int,
                 (pthread_rwlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_trywrlock, int,
                 (pthread_rwlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_timedrdlock, int,
                 (pthread_rwlock_t *, const struct timespec *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_timedwrlock, int,
                 (pthread_rwlock_t *, const struct timespec *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_unlock, int,
                 (pthread_rwlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_RWLOCK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_rwlock_destroy, int,
                 (pthread_rwlock_t *));
#endif

/* =========== Condition variable functions =========== */

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_init, int,
                 (pthread_cond_t *, const pthread_condattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_condattr_init, int,
                 (pthread_condattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_condattr_destroy, int,
                 (pthread_condattr_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_wait, int,
                 (pthread_cond_t *, pthread_mutex_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_timedwait, int,
                 (pthread_cond_t *, pthread_mutex_t *,
                  const struct timespec *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_signal, int,
                 (pthread_cond_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_broadcast, int,
                 (pthread_cond_t *));
#endif

#if GNULIB_TEST_PTHREAD_COND
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_cond_destroy, int,
                 (pthread_cond_t *));
#endif

/* =========== Thread-specific storage functions =========== */

#if GNULIB_TEST_PTHREAD_TSS
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_key_create, int,
                 (pthread_key_t *, void (*) (void *)));
#endif

#if GNULIB_TEST_PTHREAD_TSS
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_setspecific, int,
                 (pthread_key_t, const void *));
#endif

#if GNULIB_TEST_PTHREAD_TSS
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_getspecific, void *,
                 (pthread_key_t));
#endif

#if GNULIB_TEST_PTHREAD_TSS
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_key_delete, int, (pthread_key_t));
#endif

/* =========== Spinlock functions =========== */

#if GNULIB_TEST_PTHREAD_SPIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_spin_init, int,
                 (pthread_spinlock_t *, int));
#endif

#if GNULIB_TEST_PTHREAD_SPIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_spin_lock, int,
                 (pthread_spinlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_SPIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_spin_trylock, int,
                 (pthread_spinlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_SPIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_spin_unlock, int,
                 (pthread_spinlock_t *));
#endif

#if GNULIB_TEST_PTHREAD_SPIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_spin_destroy, int,
                 (pthread_spinlock_t *));
#endif


int
main ()
{
}
