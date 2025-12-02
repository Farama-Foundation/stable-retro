# Check prerequisites for compiling lib/c-stack.c.

# Copyright (C) 2002-2004, 2008-2021 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

# serial 21

AC_DEFUN([AC_SYS_XSI_STACK_OVERFLOW_HEURISTIC],
  [
   AC_REQUIRE([AC_CANONICAL_HOST])
   AC_CHECK_FUNCS_ONCE([setrlimit])
   AC_CHECK_HEADERS_ONCE([ucontext.h])

   dnl List of signals that are sent when an invalid virtual memory address
   dnl is accessed, or when the stack overflows.
   dnl Either { SIGSEGV } or { SIGSEGV, SIGBUS }.
   case "$host_os" in
     sunos4* | freebsd* | dragonfly* | midnightbsd* | openbsd* | mirbsd* | netbsd* | kfreebsd* | knetbsd*) # BSD systems
       FAULT_YIELDS_SIGBUS=1 ;;
     hpux*) # HP-UX
       FAULT_YIELDS_SIGBUS=1 ;;
     macos* | darwin*) # Mac OS X
       FAULT_YIELDS_SIGBUS=1 ;;
     gnu*) # Hurd
       FAULT_YIELDS_SIGBUS=1 ;;
     *)
       FAULT_YIELDS_SIGBUS=0 ;;
   esac
   AC_DEFINE_UNQUOTED([FAULT_YIELDS_SIGBUS], [$FAULT_YIELDS_SIGBUS],
     [Define to 1 if an invalid memory address access may yield a SIGBUS.])

   AC_CACHE_CHECK([for working C stack overflow detection],
     [gl_cv_sys_stack_overflow_works],
     [AC_RUN_IFELSE([AC_LANG_SOURCE(
           [[
            #include <unistd.h>
            #include <signal.h>
            #if HAVE_SETRLIMIT
            # include <sys/types.h>
            # include <sys/time.h>
            # include <sys/resource.h>
            #endif
            #ifndef SIGSTKSZ
            # define SIGSTKSZ 16384
            #endif

            static union
            {
              char buffer[2 * SIGSTKSZ];
              long double ld;
              long u;
              void *p;
            } alternate_signal_stack;

            static void
            segv_handler (int signo)
            {
              _exit (0);
            }

            static int
            c_stack_action ()
            {
              stack_t st;
              struct sigaction act;
              int r;

              st.ss_flags = 0;
              /* Use the midpoint to avoid Irix sigaltstack bug.  */
              st.ss_sp = alternate_signal_stack.buffer + SIGSTKSZ;
              st.ss_size = SIGSTKSZ;
              r = sigaltstack (&st, 0);
              if (r != 0)
                return 1;

              sigemptyset (&act.sa_mask);
              act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
              act.sa_handler = segv_handler;
              #if FAULT_YIELDS_SIGBUS
              if (sigaction (SIGBUS, &act, 0) < 0)
                return 2;
              #endif
              if (sigaction (SIGSEGV, &act, 0) < 0)
                return 3;
              return 0;
            }
            static volatile int *
            recurse_1 (volatile int n, volatile int *p)
            {
              if (n >= 0)
                *recurse_1 (n + 1, p) += n;
              return p;
            }
            static int
            recurse (volatile int n)
            {
              int sum = 0;
              return *recurse_1 (n, &sum);
            }
            int
            main ()
            {
              int result;
              #if HAVE_SETRLIMIT && defined RLIMIT_STACK
              /* Before starting the endless recursion, try to be friendly
                 to the user's machine.  On some Linux 2.2.x systems, there
                 is no stack limit for user processes at all.  We don't want
                 to kill such systems.  */
              struct rlimit rl;
              rl.rlim_cur = rl.rlim_max = 0x100000; /* 1 MB */
              setrlimit (RLIMIT_STACK, &rl);
              #endif

              result = c_stack_action ();
              if (result != 0)
                return result;
              return recurse (0);
            }
           ]])],
        [gl_cv_sys_stack_overflow_works=yes],
        [gl_cv_sys_stack_overflow_works=no],
        [case "$host_os" in
                   # Guess no on native Windows.
           mingw*) gl_cv_sys_stack_overflow_works="guessing no" ;;
           *)      gl_cv_sys_stack_overflow_works=cross-compiling ;;
         esac
        ])
     ])

  if test "$gl_cv_sys_stack_overflow_works" = yes; then
   AC_DEFINE([HAVE_STACK_OVERFLOW_HANDLING], [1],
     [Define to 1 if extending the stack slightly past the limit causes
      a SIGSEGV which can be handled on an alternate stack established
      with sigaltstack.])

    dnl The ss_sp field of a stack_t is, according to POSIX, the lowest address
    dnl of the memory block designated as an alternate stack. But IRIX 5.3
    dnl interprets it as the highest address!
    AC_CACHE_CHECK([for correct stack_t interpretation],
      [gl_cv_sigaltstack_low_base], [
      AC_RUN_IFELSE([
        AC_LANG_SOURCE([[
#include <stdlib.h>
#include <signal.h>
#if HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifndef SIGSTKSZ
# define SIGSTKSZ 16384
#endif
volatile char *stack_lower_bound;
volatile char *stack_upper_bound;
static void check_stack_location (volatile char *addr)
{
  if (addr >= stack_lower_bound && addr <= stack_upper_bound)
    exit (0);
  else
    exit (1);
}
static void stackoverflow_handler (int sig)
{
  char dummy;
  check_stack_location (&dummy);
}
int main ()
{
  char mystack[2 * SIGSTKSZ];
  stack_t altstack;
  struct sigaction action;
  /* Install the alternate stack.  */
  altstack.ss_sp = mystack + SIGSTKSZ;
  altstack.ss_size = SIGSTKSZ;
  stack_lower_bound = (char *) altstack.ss_sp;
  stack_upper_bound = (char *) altstack.ss_sp + altstack.ss_size - 1;
  altstack.ss_flags = 0; /* no SS_DISABLE */
  if (sigaltstack (&altstack, NULL) < 0)
    exit (2);
  /* Install the SIGSEGV handler.  */
  sigemptyset (&action.sa_mask);
  action.sa_handler = &stackoverflow_handler;
  action.sa_flags = SA_ONSTACK;
  if (sigaction (SIGSEGV, &action, (struct sigaction *) NULL) < 0)
    exit(3);
  /* Provoke a SIGSEGV.  */
  raise (SIGSEGV);
  exit (4);
}]])],
      [gl_cv_sigaltstack_low_base=yes],
      [gl_cv_sigaltstack_low_base=no],
      [gl_cv_sigaltstack_low_base=cross-compiling])])
   if test "$gl_cv_sigaltstack_low_base" = no; then
      AC_DEFINE([SIGALTSTACK_SS_REVERSED], [1],
        [Define if sigaltstack() interprets the stack_t.ss_sp field
         incorrectly, as the highest address of the alternate stack range
         rather than as the lowest address.])
    fi

   AC_CACHE_CHECK([for precise C stack overflow detection],
     [gl_cv_sys_xsi_stack_overflow_heuristic],
     [dnl On Linux/sparc64 (both in 32-bit and 64-bit mode), it would be wrong
      dnl to set HAVE_XSI_STACK_OVERFLOW_HEURISTIC to 1, because the third
      dnl argument passed to the segv_handler is a 'struct sigcontext *', not
      dnl an 'ucontext_t *'.  It would lead to a failure of test-c-stack2.sh.
      case "${host_os}--${host_cpu}" in
        linux*--sparc*)
          gl_cv_sys_xsi_stack_overflow_heuristic=no
          ;;
        *)
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE(
               [[
                #include <unistd.h>
                #include <signal.h>
                #if HAVE_UCONTEXT_H
                # include <ucontext.h>
                #endif
                #if HAVE_SETRLIMIT
                # include <sys/types.h>
                # include <sys/time.h>
                # include <sys/resource.h>
                #endif
                #ifndef SIGSTKSZ
                # define SIGSTKSZ 16384
                #endif

                static union
                {
                  char buffer[2 * SIGSTKSZ];
                  long double ld;
                  long u;
                  void *p;
                } alternate_signal_stack;

                #if STACK_DIRECTION
                # define find_stack_direction(ptr) STACK_DIRECTION
                #else
                static int
                find_stack_direction (char const *addr)
                {
                  char dummy;
                  return (! addr ? find_stack_direction (&dummy)
                          : addr < &dummy ? 1 : -1);
                }
                #endif

                static void
                segv_handler (int signo, siginfo_t *info, void *context)
                {
                  if (0 < info->si_code)
                    {
                      /* For XSI heuristics to work, we need uc_stack to
                         describe the interrupted stack (as on Solaris), and
                         not the currently executing stack (as on Linux).  */
                      ucontext_t const *user_context = context;
                      char const *stack_min = user_context->uc_stack.ss_sp;
                      size_t stack_size = user_context->uc_stack.ss_size;
                      char const *faulting_address = info->si_addr;
                      size_t s = faulting_address - stack_min;
                      size_t page_size = sysconf (_SC_PAGESIZE);
                      if (find_stack_direction (0) < 0)
                        s += page_size;
                      if (s < stack_size + page_size)
                        _exit (0);
                      _exit (4);
                    }
                  _exit (5);
                }

                static int
                c_stack_action ()
                {
                  stack_t st;
                  struct sigaction act;
                  int r;

                  st.ss_flags = 0;
                  /* Use the midpoint to avoid Irix sigaltstack bug.  */
                  st.ss_sp = alternate_signal_stack.buffer + SIGSTKSZ;
                  st.ss_size = SIGSTKSZ;
                  r = sigaltstack (&st, 0);
                  if (r != 0)
                    return 1;

                  sigemptyset (&act.sa_mask);
                  act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
                  act.sa_sigaction = segv_handler;
                  #if FAULT_YIELDS_SIGBUS
                  if (sigaction (SIGBUS, &act, 0) < 0)
                    return 2;
                  #endif
                  if (sigaction (SIGSEGV, &act, 0) < 0)
                    return 3;
                  return 0;
                }
                static volatile int *
                recurse_1 (volatile int n, volatile int *p)
                {
                  if (n >= 0)
                    *recurse_1 (n + 1, p) += n;
                  return p;
                }
                static int
                recurse (volatile int n)
                {
                  int sum = 0;
                  return *recurse_1 (n, &sum);
                }
                int
                main ()
                {
                  int result;
                  #if HAVE_SETRLIMIT && defined RLIMIT_STACK
                  /* Before starting the endless recursion, try to be friendly
                     to the user's machine.  On some Linux 2.2.x systems, there
                     is no stack limit for user processes at all.  We don't want
                     to kill such systems.  */
                  struct rlimit rl;
                  rl.rlim_cur = rl.rlim_max = 0x100000; /* 1 MB */
                  setrlimit (RLIMIT_STACK, &rl);
                  #endif

                  result = c_stack_action ();
                  if (result != 0)
                    return result;
                  return recurse (0);
                }
               ]])],
            [gl_cv_sys_xsi_stack_overflow_heuristic=yes],
            [gl_cv_sys_xsi_stack_overflow_heuristic=no],
            [gl_cv_sys_xsi_stack_overflow_heuristic=cross-compiling])
          ;;
      esac
     ])

   if test "$gl_cv_sys_xsi_stack_overflow_heuristic" = yes; then
     AC_DEFINE([HAVE_XSI_STACK_OVERFLOW_HEURISTIC], [1],
       [Define to 1 if extending the stack slightly past the limit causes
        a SIGSEGV, and an alternate stack can be established with sigaltstack,
        and the signal handler is passed a context that specifies the
        run time stack.  This behavior is defined by POSIX 1003.1-2001
        with the X/Open System Interface (XSI) option
        and is a standardized way to implement a SEGV-based stack
        overflow detection heuristic.])
   fi
  fi])


AC_DEFUN([gl_PREREQ_C_STACK],
  [AC_REQUIRE([AC_SYS_XSI_STACK_OVERFLOW_HEURISTIC])

   AC_CHECK_FUNCS_ONCE([sigaltstack])
   AC_CHECK_DECLS([sigaltstack], , , [[#include <signal.h>]])

   AC_CHECK_HEADERS_ONCE([ucontext.h])

   AC_CHECK_TYPES([stack_t], , , [[#include <signal.h>]])

   dnl c-stack does not need -lsigsegv if the system has XSI heuristics.
   if test "$gl_cv_sys_xsi_stack_overflow_heuristic" != yes; then
     gl_LIBSIGSEGV
     if test "$gl_cv_lib_sigsegv" = yes; then
       AC_SUBST([LIBCSTACK], [$LIBSIGSEGV])
       AC_SUBST([LTLIBCSTACK], [$LTLIBSIGSEGV])
     fi
   fi
])

AC_DEFUN([gl_C_STACK],
[
  dnl Prerequisites of lib/c-stack.c.
  gl_PREREQ_C_STACK
])
