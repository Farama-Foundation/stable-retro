#! /bin/sh

# func_exit STATUS
# exit with status
func_exit ()
{
  (exit $1); exit $1
}

# func_fatal_error message
# outputs to stderr a fatal error message, and terminates the program.
func_fatal_error ()
{
  echo "test-echo.sh: *** $1" 1>&2
  echo "test-echo.sh: *** Stop." 1>&2
  func_exit 1
}

# Ensure an 'echo' command that does not interpret backslashes.
# Test cases:
#   echo '\n' | wc -l                 prints 1 when OK, 2 when KO
#   echo '\t' | grep t > /dev/null    has return code 0 when OK, 1 when KO
# This problem is a weird heritage from SVR4. BSD got it right (except that
# BSD echo interprets '-n' as an option, which is also not desirable).
# Nowadays the problem occurs in 4 situations:
# - in bash, when the shell option xpg_echo is set (bash >= 2.04)
#            or when it was built with --enable-usg-echo-default (bash >= 2.0)
#            or when it was built with DEFAULT_ECHO_TO_USG (bash < 2.0),
# - in zsh, when sh-emulation is not set,
# - in ksh (e.g. AIX /bin/sh and Solaris /usr/xpg4/bin/sh are ksh instances,
#           and HP-UX /bin/sh and IRIX /bin/sh behave similarly),
# - in Solaris /bin/sh and OSF/1 /bin/sh.
# We try the following workarounds:
# - for all: respawn using $CONFIG_SHELL if that is set and works.
# - for bash >= 2.04: unset the shell option xpg_echo.
# - for bash >= 2.0: define echo to a function that uses the printf built-in.
# - for bash < 2.0: define echo to a function that uses cat of a here document.
# - for zsh: turn sh-emulation on.
# - for ksh: alias echo to 'print -r'.
# - for ksh: alias echo to a function that uses cat of a here document.
# - for Solaris /bin/sh and OSF/1 /bin/sh: respawn using /bin/ksh and rely on
#   the ksh workaround.
# - otherwise: respawn using /bin/sh and rely on the workarounds.
# When respawning, we pass --no-reexec as first argument, so as to avoid
# turning this script into a fork bomb in unlucky situations.
have_echo=
if echo '\t' | grep t > /dev/null; then
  have_echo=yes # Lucky!
fi
# Try the workarounds.
# Respawn using $CONFIG_SHELL if that is set and works.
if test -z "$have_echo" \
   && test "X$1" != "X--no-reexec" \
   && test -n "$CONFIG_SHELL" \
   && test -f "$CONFIG_SHELL" \
   && $CONFIG_SHELL -c 'echo '\t' | grep t > /dev/null'; then
  exec $CONFIG_SHELL "$0" --no-reexec "$@"
  exit 127
fi
# For bash >= 2.04: unset the shell option xpg_echo.
if test -z "$have_echo" \
   && test -n "$BASH_VERSION" \
   && (shopt -o xpg_echo; echo '\t' | grep t > /dev/null) 2>/dev/null; then
  shopt -o xpg_echo
  have_echo=yes
fi
# For bash >= 2.0: define echo to a function that uses the printf built-in.
# For bash < 2.0: define echo to a function that uses cat of a here document.
# (There is no win in using 'printf' over 'cat' if it is not a shell built-in.)
if test -z "$have_echo" \
   && test -n "$BASH_VERSION"; then \
  if type printf 2>/dev/null | grep / > /dev/null; then
    # 'printf' is not a shell built-in.
echo ()
{
cat <<EOF
$*
EOF
}
  else
    # 'printf' is a shell built-in.
echo ()
{
  printf '%s\n' "$*"
}
  fi
  if echo '\t' | grep t > /dev/null; then
    have_echo=yes
  fi
fi
# For zsh: turn sh-emulation on.
if test -z "$have_echo" \
   && test -n "$ZSH_VERSION" \
   && (emulate sh) >/dev/null 2>&1; then
  emulate sh
fi
# For ksh: alias echo to 'print -r'.
if test -z "$have_echo" \
   && (type print) >/dev/null 2>&1; then
  # A 'print' command exists.
  if type print 2>/dev/null | grep / > /dev/null; then
    :
  else
    # 'print' is a shell built-in.
    if (print -r '\told' | grep told > /dev/null) 2>/dev/null; then
      # 'print' is the ksh shell built-in.
      alias echo='print -r'
    fi
  fi
fi
if test -z "$have_echo" \
   && echo '\t' | grep t > /dev/null; then
  have_echo=yes
fi
# For ksh: alias echo to a function that uses cat of a here document.
# The ksh manual page says:
#   "Aliasing is performed when scripts are read, not while they are executed.
#    Therefore, for an alias to take effect, the alias definition command has
#    to be executed before the command which references the alias is read."
# Because of this, we have to play strange tricks with have_echo, to ensure
# that the top-level statement containing the test starts after the 'alias'
# command.
if test -z "$have_echo"; then
bsd_echo ()
{
cat <<EOF
$*
EOF
}
alias echo=bsd_echo 2>/dev/null
fi
if test -z "$have_echo" \
   && echo '\t' | grep t > /dev/null; then
  have_echo=yes
fi
if test -z "$have_echo"; then
  unalias echo 2>/dev/null
fi
# For Solaris /bin/sh and OSF/1 /bin/sh: respawn using /bin/ksh.
if test -z "$have_echo" \
   && test "X$1" != "X--no-reexec" \
   && test -f /bin/ksh; then
  exec /bin/ksh "$0" --no-reexec "$@"
  exit 127
fi
# Otherwise: respawn using /bin/sh.
if test -z "$have_echo" \
   && test "X$1" != "X--no-reexec" \
   && test -f /bin/sh; then
  exec /bin/sh "$0" --no-reexec "$@"
  exit 127
fi
if test -z "$have_echo"; then
  func_fatal_error "Shell does not support 'echo' correctly. Please install GNU bash and set the environment variable CONFIG_SHELL to point to it."
fi
if echo '\t' | grep t > /dev/null; then
  : # Works fine now.
else
  func_fatal_error "Shell does not support 'echo' correctly. Workaround does not work. Please report this as a bug to bug-gnulib@gnu.org."
fi
if test "X$1" = "X--no-reexec"; then
  shift
fi

# This command determines the exit code.
echo '\t' | grep t > /dev/null
