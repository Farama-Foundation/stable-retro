/* Test of <threads.h> substitute in C++ mode.
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

#include <threads.h>

#include "signature.h"


#if GNULIB_TEST_THRD
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_create, int,
                 (thrd_t *, thrd_start_t, void *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_current, thrd_t, (void));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_equal, int, (thrd_t, thrd_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_sleep, int,
                 (const struct timespec *, struct timespec *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_yield, void, (void));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_detach, int, (thrd_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_join, int, (thrd_t, int *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::thrd_exit, void, (int));
#endif

#if GNULIB_TEST_MTX
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_init, int, (mtx_t *, int));
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_lock, int, (mtx_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_trylock, int, (mtx_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_timedlock, int,
                 (mtx_t *, const struct timespec *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_unlock, int, (mtx_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::mtx_destroy, void, (mtx_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::call_once, void,
                 (once_flag *, void (*) (void)));
#endif

#if GNULIB_TEST_CND
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_init, int, (cnd_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_wait, int, (cnd_t *, mtx_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_timedwait, int,
                 (cnd_t *, mtx_t *, const struct timespec *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_signal, int, (cnd_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_broadcast, int, (cnd_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::cnd_destroy, void, (cnd_t *));
#endif

#if GNULIB_TEST_TSS
SIGNATURE_CHECK (GNULIB_NAMESPACE::tss_create, int, (tss_t *, tss_dtor_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::tss_set, int, (tss_t, void *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::tss_get, void *, (tss_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::tss_delete, void, (tss_t));
#endif


int
main ()
{
}
