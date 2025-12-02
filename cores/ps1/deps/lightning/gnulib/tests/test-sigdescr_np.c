/* Test of sigdescr_np() function.

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
SIGNATURE_CHECK (sigdescr_np, const char *, (int));

#include <signal.h>

#include "macros.h"

int
main (void)
{
  /* Signals specified by ISO C.  */
  ASSERT (strcmp (sigdescr_np (SIGABRT), "Aborted") == 0);
  ASSERT (strcmp (sigdescr_np (SIGFPE), "Floating point exception") == 0
          || strcmp (sigdescr_np (SIGFPE), "Arithmetic exception") == 0);
  ASSERT (strcmp (sigdescr_np (SIGILL), "Illegal instruction") == 0);
  ASSERT (strcmp (sigdescr_np (SIGINT), "Interrupt") == 0);
  ASSERT (strcmp (sigdescr_np (SIGSEGV), "Segmentation fault") == 0);
  ASSERT (strcmp (sigdescr_np (SIGTERM), "Terminated") == 0);

  /* Signals specified by POSIX.
     <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html>  */
  #ifdef SIGALRM
  ASSERT (strcmp (sigdescr_np (SIGALRM), "Alarm clock") == 0);
  #endif
  #ifdef SIGBUS
  ASSERT (strcmp (sigdescr_np (SIGBUS), "Bus error") == 0);
  #endif
  #ifdef SIGCHLD
  ASSERT (strcmp (sigdescr_np (SIGCHLD), "Child exited") == 0 /* glibc */
          || strcmp (sigdescr_np (SIGCHLD), "Child stopped or exited") == 0);
  #endif
  #ifdef SIGCONT
  ASSERT (strcmp (sigdescr_np (SIGCONT), "Continued") == 0);
  #endif
  #ifdef SIGHUP
  ASSERT (strcmp (sigdescr_np (SIGHUP), "Hangup") == 0);
  #endif
  #ifdef SIGKILL
  ASSERT (strcmp (sigdescr_np (SIGKILL), "Killed") == 0);
  #endif
  #ifdef SIGPIPE
  ASSERT (strcmp (sigdescr_np (SIGPIPE), "Broken pipe") == 0);
  #endif
  #ifdef SIGQUIT
  ASSERT (strcmp (sigdescr_np (SIGQUIT), "Quit") == 0);
  #endif
  #ifdef SIGSTOP
  ASSERT (strcmp (sigdescr_np (SIGSTOP), "Stopped (signal)") == 0);
  #endif
  #ifdef SIGTSTP
  ASSERT (strcmp (sigdescr_np (SIGTSTP), "Stopped") == 0);
  #endif
  #ifdef SIGTTIN
  ASSERT (strcmp (sigdescr_np (SIGTTIN), "Stopped (tty input)") == 0);
  #endif
  #ifdef SIGTTOU
  ASSERT (strcmp (sigdescr_np (SIGTTOU), "Stopped (tty output)") == 0);
  #endif
  #ifdef SIGUSR1
  ASSERT (strcmp (sigdescr_np (SIGUSR1), "User defined signal 1") == 0);
  #endif
  #ifdef SIGUSR2
  ASSERT (strcmp (sigdescr_np (SIGUSR2), "User defined signal 2") == 0);
  #endif
  #ifdef SIGPOLL
  ASSERT (strcmp (sigdescr_np (SIGPOLL), "I/O possible") == 0);
  #endif
  #ifdef SIGPROF
  ASSERT (strcmp (sigdescr_np (SIGPROF), "Profiling timer expired") == 0);
  #endif
  #ifdef SIGSYS
  ASSERT (strcmp (sigdescr_np (SIGSYS), "Bad system call") == 0);
  #endif
  #ifdef SIGTRAP
  ASSERT (strcmp (sigdescr_np (SIGTRAP), "Trace/breakpoint trap") == 0);
  #endif
  #ifdef SIGURG
  ASSERT (strcmp (sigdescr_np (SIGURG), "Urgent I/O condition") == 0);
  #endif
  #ifdef SIGVTALRM
  ASSERT (strcmp (sigdescr_np (SIGVTALRM), "Virtual timer expired") == 0);
  #endif
  #ifdef SIGXCPU
  ASSERT (strcmp (sigdescr_np (SIGXCPU), "CPU time limit exceeded") == 0);
  #endif
  #ifdef SIGXFSZ
  ASSERT (strcmp (sigdescr_np (SIGXFSZ), "File size limit exceeded") == 0);
  #endif

  /* Other signals on other systems.  */
  /* native Windows */
  #ifdef SIGBREAK
  ASSERT (strcmp (sigdescr_np (SIGBREAK), "Ctrl-Break") == 0);
  #endif
  /* IRIX */
  #ifdef SIGCKPT
  ASSERT (strcmp (sigdescr_np (SIGCKPT), "Checkpoint") == 0);
  #endif
  /* AIX */
  #ifdef SIGCPUFAIL
  ASSERT (strcmp (sigdescr_np (SIGCPUFAIL), "CPU going down") == 0);
  #endif
  /* AIX */
  #ifdef SIGDANGER
  ASSERT (strcmp (sigdescr_np (SIGDANGER), "Swap space nearly exhausted") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix, AIX, IRIX, Cygwin, mingw */
  #ifdef SIGEMT
  ASSERT (strcmp (sigdescr_np (SIGEMT), "Instruction emulation needed") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix */
  #ifdef SIGINFO
  ASSERT (strcmp (sigdescr_np (SIGINFO), "Information request") == 0);
  #endif
  /* AIX */
  #ifdef SIGKAP
  ASSERT (strcmp (sigdescr_np (SIGKAP), "Keep Alive Poll") == 0);
  #endif
  /* Haiku */
  #ifdef SIGKILLTHR
  ASSERT (strcmp (sigdescr_np (SIGKILLTHR), "Kill thread") == 0);
  #endif
  /* Minix */
  #ifdef SIGKMEM
  ASSERT (strcmp (sigdescr_np (SIGKMEM), "Kernel memory request") == 0);
  #endif
  /* Minix */
  #ifdef SIGKMESS
  ASSERT (strcmp (sigdescr_np (SIGKMESS), "Kernel message") == 0);
  #endif
  /* Minix */
  #ifdef SIGKSIG
  ASSERT (strcmp (sigdescr_np (SIGKSIG), "Kernel signal") == 0);
  #endif
  /* Minix */
  #ifdef SIGKSIGSM
  ASSERT (strcmp (sigdescr_np (SIGKSIGSM), "Kernel signal for signal manager") == 0);
  #endif
  /* FreeBSD */
  #ifdef SIGLIBRT
  ASSERT (strcmp (sigdescr_np (SIGLIBRT), "Real-time library interrupt") == 0);
  #endif
  /* AIX */
  #ifdef SIGMIGRATE
  ASSERT (strcmp (sigdescr_np (SIGMIGRATE), "Process migration") == 0);
  #endif
  /* AIX */
  #ifdef SIGMSG
  ASSERT (strcmp (sigdescr_np (SIGMSG), "Message in the ring") == 0);
  #endif
  /* AIX */
  #ifdef SIGPRE
  ASSERT (strcmp (sigdescr_np (SIGPRE), "Programmed exception") == 0);
  #endif
  /* IRIX */
  #ifdef SIGPTINTR
  ASSERT (strcmp (sigdescr_np (SIGPTINTR), "Pthread interrupt") == 0);
  #endif
  /* IRIX */
  #ifdef SIGPTRESCHED
  ASSERT (strcmp (sigdescr_np (SIGPTRESCHED), "Pthread rescheduling") == 0);
  #endif
  /* Linux, NetBSD, Minix, AIX, IRIX, Cygwin */
  #ifdef SIGPWR
  ASSERT (strcmp (sigdescr_np (SIGPWR), "Power failure") == 0);
  #endif
  /* AIX */
  #ifdef SIGRECONFIG
  ASSERT (strcmp (sigdescr_np (SIGRECONFIG), "Dynamic logical partitioning changed") == 0);
  #endif
  /* AIX */
  #ifdef SIGRECOVERY
  ASSERT (strcmp (sigdescr_np (SIGRECOVERY), "Kernel recovery") == 0);
  #endif
  /* IRIX */
  #ifdef SIGRESTART
  ASSERT (strcmp (sigdescr_np (SIGRESTART), "Checkpoint restart") == 0);
  #endif
  /* AIX */
  #ifdef SIGRETRACT
  ASSERT (strcmp (sigdescr_np (SIGRETRACT), "Retracting Keep Alive Poll") == 0);
  #endif
  /* AIX */
  #ifdef SIGSAK
  ASSERT (strcmp (sigdescr_np (SIGSAK), "Secure Attention Key") == 0);
  #endif
  /* Minix */
  #ifdef SIGSNDELAY
  ASSERT (strcmp (sigdescr_np (SIGSNDELAY), "Done sending message") == 0);
  #endif
  /* AIX */
  #ifdef SIGSOUND
  ASSERT (strcmp (sigdescr_np (SIGSOUND), "Sound configuration changed") == 0);
  #endif
  /* Linux */
  #ifdef SIGSTKFLT
  ASSERT (strcmp (sigdescr_np (SIGSTKFLT), "Stack fault") == 0);
  #endif
  /* AIX */
  #ifdef SIGSYSERROR
  ASSERT (strcmp (sigdescr_np (SIGSYSERROR), "Kernel error") == 0);
  #endif
  /* AIX */
  #ifdef SIGTALRM
  ASSERT (strcmp (sigdescr_np (SIGTALRM), "Thread alarm clock") == 0);
  #endif
  /* FreeBSD, OpenBSD */
  #ifdef SIGTHR
  ASSERT (strcmp (sigdescr_np (SIGTHR), "Thread library interrupt") == 0);
  #endif
  /* IRIX */
  #ifdef SIGUME
  ASSERT (strcmp (sigdescr_np (SIGUME), "Uncorrectable memory error") == 0);
  #endif
  /* AIX */
  #ifdef SIGVIRT
  ASSERT (strcmp (sigdescr_np (SIGVIRT), "Virtual time alarm clock") == 0);
  #endif
  /* AIX */
  #ifdef SIGWAITING
  ASSERT (strcmp (sigdescr_np (SIGWAITING), "Thread waiting") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix, AIX, IRIX, Cygwin, Haiku */
  #ifdef SIGWINCH
  ASSERT (strcmp (sigdescr_np (SIGWINCH), "Window changed") == 0 /* glibc */
          || strcmp (sigdescr_np (SIGWINCH), "Window size changed") == 0);
  #endif

  ASSERT (sigdescr_np (-714) == NULL);

  return 0;
}
