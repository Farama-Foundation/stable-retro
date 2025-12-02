/* Test of <spawn.h> substitute in C++ mode.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#include <spawn.h>

#include "signature.h"


#if GNULIB_TEST_POSIX_SPAWN
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn, int,
                 (pid_t *, const char *, const posix_spawn_file_actions_t *,
                  const posix_spawnattr_t *, char *const[], char *const[]));
#endif

#if GNULIB_TEST_POSIX_SPAWNP
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnp, int,
                 (pid_t *, const char *, const posix_spawn_file_actions_t *,
                  const posix_spawnattr_t *, char *const[], char *const[]));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_INIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_init, int,
                 (posix_spawnattr_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_DESTROY
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_destroy, int,
                 (posix_spawnattr_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETSIGDEFAULT
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getsigdefault, int,
                 (const posix_spawnattr_t *, sigset_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETSIGDEFAULT
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setsigdefault, int,
                 (posix_spawnattr_t *, const sigset_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETSIGMASK
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getsigmask, int,
                 (const posix_spawnattr_t *, sigset_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETSIGMASK
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setsigmask, int,
                 (posix_spawnattr_t *, const sigset_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETFLAGS
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getflags, int,
                 (const posix_spawnattr_t *, short int *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETFLAGS
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setflags, int,
                 (posix_spawnattr_t *, short int));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETPGROUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getpgroup, int,
                 (const posix_spawnattr_t *, pid_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETPGROUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setpgroup, int,
                 (posix_spawnattr_t *, pid_t));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETSCHEDPOLICY
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getschedpolicy, int,
                 (const posix_spawnattr_t *, int *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETSCHEDPOLICY
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setschedpolicy, int,
                 (posix_spawnattr_t *, int));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_GETSCHEDPARAM
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_getschedparam, int,
                 (const posix_spawnattr_t *, struct sched_param *));
#endif

#if GNULIB_TEST_POSIX_SPAWNATTR_SETSCHEDPARAM
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawnattr_setschedparam, int,
                 (posix_spawnattr_t *, const struct sched_param *));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_INIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_init, int,
                 (posix_spawn_file_actions_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_DESTROY
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_destroy, int,
                 (posix_spawn_file_actions_t *));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_ADDOPEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_addopen, int,
                 (posix_spawn_file_actions_t *, int, const char *, int,
                  mode_t));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSE
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_addclose, int,
                 (posix_spawn_file_actions_t *, int));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_ADDDUP2
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_adddup2, int,
                 (posix_spawn_file_actions_t *, int, int));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_ADDCHDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_addchdir, int,
                 (posix_spawn_file_actions_t *, const char *));
#endif

#if GNULIB_TEST_POSIX_SPAWN_FILE_ACTIONS_ADDFCHDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::posix_spawn_file_actions_addfchdir, int,
                 (posix_spawn_file_actions_t *, int));
#endif


int
main ()
{
}
