/* Test of sigabbrev_np() function.

   Copyright (C) 2020-2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (sigabbrev_np, const char *, (int));

#include <signal.h>

#include "macros.h"

int
main (void)
{
  /* Signals specified by ISO C.  */
  ASSERT (strcmp (sigabbrev_np (SIGABRT), "ABRT") == 0);
  ASSERT (strcmp (sigabbrev_np (SIGFPE), "FPE") == 0);
  ASSERT (strcmp (sigabbrev_np (SIGILL), "ILL") == 0);
  ASSERT (strcmp (sigabbrev_np (SIGINT), "INT") == 0);
  ASSERT (strcmp (sigabbrev_np (SIGSEGV), "SEGV") == 0);
  ASSERT (strcmp (sigabbrev_np (SIGTERM), "TERM") == 0);

  /* Signals specified by POSIX.
     <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html>  */
  #ifdef SIGALRM
  ASSERT (strcmp (sigabbrev_np (SIGALRM), "ALRM") == 0);
  #endif
  #ifdef SIGBUS
  ASSERT (strcmp (sigabbrev_np (SIGBUS), "BUS") == 0);
  #endif
  #ifdef SIGCHLD
  ASSERT (strcmp (sigabbrev_np (SIGCHLD), "CHLD") == 0);
  #endif
  #ifdef SIGCONT
  ASSERT (strcmp (sigabbrev_np (SIGCONT), "CONT") == 0);
  #endif
  #ifdef SIGHUP
  ASSERT (strcmp (sigabbrev_np (SIGHUP), "HUP") == 0);
  #endif
  #ifdef SIGKILL
  ASSERT (strcmp (sigabbrev_np (SIGKILL), "KILL") == 0);
  #endif
  #ifdef SIGPIPE
  ASSERT (strcmp (sigabbrev_np (SIGPIPE), "PIPE") == 0);
  #endif
  #ifdef SIGQUIT
  ASSERT (strcmp (sigabbrev_np (SIGQUIT), "QUIT") == 0);
  #endif
  #ifdef SIGSTOP
  ASSERT (strcmp (sigabbrev_np (SIGSTOP), "STOP") == 0);
  #endif
  #ifdef SIGTSTP
  ASSERT (strcmp (sigabbrev_np (SIGTSTP), "TSTP") == 0);
  #endif
  #ifdef SIGTTIN
  ASSERT (strcmp (sigabbrev_np (SIGTTIN), "TTIN") == 0);
  #endif
  #ifdef SIGTTOU
  ASSERT (strcmp (sigabbrev_np (SIGTTOU), "TTOU") == 0);
  #endif
  #ifdef SIGUSR1
  ASSERT (strcmp (sigabbrev_np (SIGUSR1), "USR1") == 0);
  #endif
  #ifdef SIGUSR2
  ASSERT (strcmp (sigabbrev_np (SIGUSR2), "USR2") == 0);
  #endif
  #ifdef SIGPOLL
  ASSERT (strcmp (sigabbrev_np (SIGPOLL), "POLL") == 0);
  #endif
  #ifdef SIGPROF
  ASSERT (strcmp (sigabbrev_np (SIGPROF), "PROF") == 0);
  #endif
  #ifdef SIGSYS
  ASSERT (strcmp (sigabbrev_np (SIGSYS), "SYS") == 0);
  #endif
  #ifdef SIGTRAP
  ASSERT (strcmp (sigabbrev_np (SIGTRAP), "TRAP") == 0);
  #endif
  #ifdef SIGURG
  ASSERT (strcmp (sigabbrev_np (SIGURG), "URG") == 0);
  #endif
  #ifdef SIGVTALRM
  ASSERT (strcmp (sigabbrev_np (SIGVTALRM), "VTALRM") == 0);
  #endif
  #ifdef SIGXCPU
  ASSERT (strcmp (sigabbrev_np (SIGXCPU), "XCPU") == 0);
  #endif
  #ifdef SIGXFSZ
  ASSERT (strcmp (sigabbrev_np (SIGXFSZ), "XFSZ") == 0);
  #endif

  /* Other signals on other systems.  */
  /* native Windows */
  #ifdef SIGBREAK
  ASSERT (strcmp (sigabbrev_np (SIGBREAK), "BREAK") == 0);
  #endif
  /* IRIX */
  #ifdef SIGCKPT
  ASSERT (strcmp (sigabbrev_np (SIGCKPT), "CKPT") == 0);
  #endif
  /* AIX */
  #ifdef SIGCPUFAIL
  ASSERT (strcmp (sigabbrev_np (SIGCPUFAIL), "CPUFAIL") == 0);
  #endif
  /* AIX */
  #ifdef SIGDANGER
  ASSERT (strcmp (sigabbrev_np (SIGDANGER), "DANGER") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix, AIX, IRIX, Cygwin, mingw */
  #ifdef SIGEMT
  ASSERT (strcmp (sigabbrev_np (SIGEMT), "EMT") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix */
  #ifdef SIGINFO
  ASSERT (strcmp (sigabbrev_np (SIGINFO), "INFO") == 0);
  #endif
  /* AIX */
  #ifdef SIGKAP
  ASSERT (strcmp (sigabbrev_np (SIGKAP), "KAP") == 0);
  #endif
  /* Haiku */
  #ifdef SIGKILLTHR
  ASSERT (strcmp (sigabbrev_np (SIGKILLTHR), "KILLTHR") == 0);
  #endif
  /* Minix */
  #ifdef SIGKMEM
  ASSERT (strcmp (sigabbrev_np (SIGKMEM), "KMEM") == 0);
  #endif
  /* Minix */
  #ifdef SIGKMESS
  ASSERT (strcmp (sigabbrev_np (SIGKMESS), "KMESS") == 0);
  #endif
  /* Minix */
  #ifdef SIGKSIG
  ASSERT (strcmp (sigabbrev_np (SIGKSIG), "KSIG") == 0);
  #endif
  /* Minix */
  #ifdef SIGKSIGSM
  ASSERT (strcmp (sigabbrev_np (SIGKSIGSM), "KSIGSM") == 0);
  #endif
  /* FreeBSD */
  #ifdef SIGLIBRT
  ASSERT (strcmp (sigabbrev_np (SIGLIBRT), "LIBRT") == 0);
  #endif
  /* AIX */
  #ifdef SIGMIGRATE
  ASSERT (strcmp (sigabbrev_np (SIGMIGRATE), "MIGRATE") == 0);
  #endif
  /* AIX */
  #ifdef SIGMSG
  ASSERT (strcmp (sigabbrev_np (SIGMSG), "MSG") == 0);
  #endif
  /* AIX */
  #ifdef SIGPRE
  ASSERT (strcmp (sigabbrev_np (SIGPRE), "PRE") == 0);
  #endif
  /* IRIX */
  #ifdef SIGPTINTR
  ASSERT (strcmp (sigabbrev_np (SIGPTINTR), "PTINTR") == 0);
  #endif
  /* IRIX */
  #ifdef SIGPTRESCHED
  ASSERT (strcmp (sigabbrev_np (SIGPTRESCHED), "PTRESCHED") == 0);
  #endif
  /* Linux, NetBSD, Minix, AIX, IRIX, Cygwin */
  #ifdef SIGPWR
  ASSERT (strcmp (sigabbrev_np (SIGPWR), "PWR") == 0);
  #endif
  /* AIX */
  #ifdef SIGRECONFIG
  ASSERT (strcmp (sigabbrev_np (SIGRECONFIG), "RECONFIG") == 0);
  #endif
  /* AIX */
  #ifdef SIGRECOVERY
  ASSERT (strcmp (sigabbrev_np (SIGRECOVERY), "RECOVERY") == 0);
  #endif
  /* IRIX */
  #ifdef SIGRESTART
  ASSERT (strcmp (sigabbrev_np (SIGRESTART), "RESTART") == 0);
  #endif
  /* AIX */
  #ifdef SIGRETRACT
  ASSERT (strcmp (sigabbrev_np (SIGRETRACT), "RETRACT") == 0);
  #endif
  /* AIX */
  #ifdef SIGSAK
  ASSERT (strcmp (sigabbrev_np (SIGSAK), "SAK") == 0);
  #endif
  /* Minix */
  #ifdef SIGSNDELAY
  ASSERT (strcmp (sigabbrev_np (SIGSNDELAY), "SNDELAY") == 0);
  #endif
  /* AIX */
  #ifdef SIGSOUND
  ASSERT (strcmp (sigabbrev_np (SIGSOUND), "SOUND") == 0);
  #endif
  /* Linux */
  #ifdef SIGSTKFLT
  ASSERT (strcmp (sigabbrev_np (SIGSTKFLT), "STKFLT") == 0);
  #endif
  /* AIX */
  #ifdef SIGSYSERROR
  ASSERT (strcmp (sigabbrev_np (SIGSYSERROR), "SYSERROR") == 0);
  #endif
  /* AIX */
  #ifdef SIGTALRM
  ASSERT (strcmp (sigabbrev_np (SIGTALRM), "TALRM") == 0);
  #endif
  /* FreeBSD, OpenBSD */
  #ifdef SIGTHR
  ASSERT (strcmp (sigabbrev_np (SIGTHR), "THR") == 0);
  #endif
  /* IRIX */
  #ifdef SIGUME
  ASSERT (strcmp (sigabbrev_np (SIGUME), "UME") == 0);
  #endif
  /* AIX */
  #ifdef SIGVIRT
  ASSERT (strcmp (sigabbrev_np (SIGVIRT), "VIRT") == 0);
  #endif
  /* AIX */
  #ifdef SIGWAITING
  ASSERT (strcmp (sigabbrev_np (SIGWAITING), "WAITING") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix, AIX, IRIX, Cygwin, Haiku */
  #ifdef SIGWINCH
  ASSERT (strcmp (sigabbrev_np (SIGWINCH), "WINCH") == 0);
  #endif

  ASSERT (sigabbrev_np (-714) == NULL);

  return 0;
}
