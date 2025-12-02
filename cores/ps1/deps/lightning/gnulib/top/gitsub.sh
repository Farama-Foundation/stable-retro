#! /bin/sh
#
# Copyright (C) 2019-2021 Free Software Foundation, Inc.
# Written by Bruno Haible <bruno@clisp.org>, 2019.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Program that manages the subdirectories of a git checkout of a package
# that come from other packages (called "dependency packages").
#
# This program is similar in spirit to 'git submodule', with three
# essential differences:
#
#   1) Its options are easy to remember, and do not require knowledge of
#      'git submodule'.
#
#   2) The developer may choose to work on a different checkout for each
#      dependency package.  This is important when the developer is
#      preparing simultaneous changes to the package and the dependency
#      package, or is using the dependency package in several packages.
#
#      The developer indicates this different checkout by setting the
#      environment variable <SUBDIR>_SRCDIR (e.g. GNULIB_SRCDIR) to point to it.
#
#   3) The package maintainer may choose to use or not use git submodules.
#
#      The advantages of management through a git submodule are:
#        - Changes to the dependency package cannot suddenly break your package.
#          In other words, when there is an incompatible change that will cause
#          a breakage, you can fix things at your pace; you are not forced to
#          cope with such breakages in an emergency.
#        - When you need to make a change as a response to a change in the
#          dependency package, your co-developers cannot accidentally mix things
#          up (for example, use a combination of your newest change with an
#          older version of the dependency package).
#
#      The advantages of management without a git submodule (just as a plain
#      subdirectory, let's call it a "subcheckout") are:
#        - The simplicity: you are conceptually always using the newest revision
#          of the dependency package.
#        - You don't have to remember to periodially upgrade the dependency.
#          Upgrading the dependency is an implicit operation.

# This program is meant to be copied to the top-level directory of the package,
# together with a configuration file.  The configuration is supposed to be
# named '.gitmodules' and to define:
# * The git submodules, as described in "man 5 gitmodules" or
#   <https://git-scm.com/docs/gitmodules>.  For example:
#
#       [submodule "gnulib"]
#               url = https://git.savannah.gnu.org/git/gnulib.git
#               path = gnulib
#
#   You don't add this piece of configuration to .gitmodules manually.  Instead,
#   you would invoke
#     $ git submodule add --name "gnulib" -- https://git.savannah.gnu.org/git/gnulib.git gnulib
#
# * The subdirectories that are not git submodules, in a similar syntax.  For
#   example:
#
#       [subcheckout "gnulib"]
#               url = https://git.savannah.gnu.org/git/gnulib.git
#               path = gnulib
#
# Here the URL is the one used for anonymous checkouts of the dependency
# package. If the developer needs a checkout with write access, they can
# either set the GNULIB_SRCDIR environment variable to point to that checkout
# or modify the gnulib/.git/config file to enter a different URL.

scriptname="$0"
scriptversion='2019-04-01'
nl='
'
IFS=" ""	$nl"

# func_usage
# outputs to stdout the --help usage message.
func_usage ()
{
  echo "\
Usage: gitsub.sh pull [SUBDIR]
       gitsub.sh upgrade [SUBDIR]
       gitsub.sh checkout SUBDIR REVISION

Operations:

gitsub.sh pull [GIT_OPTIONS] [SUBDIR]
        You should perform this operation after 'git clone ...' and after
        every 'git pull'.
        It brings your checkout in sync with what the other developers of
        your package have committed and pushed.
        If an environment variable <SUBDIR>_SRCDIR is set, with a non-empty
        value, nothing is done for this SUBDIR.
        Supported GIT_OPTIONS (for expert git users) are:
          --reference <repository>
          --depth <depth>
          --recursive
        If no SUBDIR is specified, the operation applies to all dependencies.

gitsub.sh upgrade [SUBDIR]
        You should perform this operation periodically, to ensure currency
        of the dependency package revisions that you use.
        This operation pulls and checks out the changes that the developers
        of the dependency package have committed and pushed.
        If an environment variable <SUBDIR>_SRCDIR is set, with a non-empty
        value, nothing is done for this SUBDIR.
        If no SUBDIR is specified, the operation applies to all dependencies.

gitsub.sh checkout SUBDIR REVISION
        Checks out a specific revision for a dependency package.
        If an environment variable <SUBDIR>_SRCDIR is set, with a non-empty
        value, this operation fails.

This script requires the git program in the PATH and an internet connection.
"
}

# func_version
# outputs to stdout the --version message.
func_version ()
{
  year=`echo "$scriptversion" | sed -e 's/^\(....\)-.*/\1/'`
  echo "\
gitsub.sh (GNU gnulib) $scriptversion
Copyright (C) 2019-$year Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
"
  printf "Written by %s.\n" "Bruno Haible"
}

# func_fatal_error message
# outputs to stderr a fatal error message, and terminates the program.
# Input:
# - scriptname               name of this program
func_fatal_error ()
{
  echo "$scriptname: *** $1" 1>&2
  echo "$scriptname: *** Stop." 1>&2
  exit 1
}

# func_warning message
# Outputs to stderr a warning message,
func_warning ()
{
  echo "gitsub.sh: warning: $1" 1>&2
}

# func_note message
# Outputs to stdout a note message,
func_note ()
{
  echo "gitsub.sh: note: $1"
}

# Unset CDPATH.  Otherwise, output from 'cd dir' can surprise callers.
(unset CDPATH) >/dev/null 2>&1 && unset CDPATH

# Command-line option processing.
mode=
while test $# -gt 0; do
  case "$1" in
    --help | --hel | --he | --h )
      func_usage
      exit $? ;;
    --version | --versio | --versi | --vers | --ver | --ve | --v )
      func_version
      exit $? ;;
    -- )
      # Stop option processing
      shift
      break ;;
    -* )
      echo "gitsub.sh: unknown option $1" 1>&2
      echo "Try 'gitsub.sh --help' for more information." 1>&2
      exit 1 ;;
    * )
      break ;;
  esac
done
if test $# = 0; then
  echo "gitsub.sh: missing operation argument" 1>&2
  echo "Try 'gitsub.sh --help' for more information." 1>&2
  exit 1
fi
case "$1" in
  pull | upgrade | checkout )
    mode="$1"
    shift ;;
  *)
    echo "gitsub.sh: unknown operation '$1'" 1>&2
    echo "Try 'gitsub.sh --help' for more information." 1>&2
    exit 1 ;;
esac
if { test $mode = upgrade && test $# -gt 1; } \
   || { test $mode = checkout && test $# -gt 2; }; then
  echo "gitsub.sh: too many arguments in '$mode' mode" 1>&2
  echo "Try 'gitsub.sh --help' for more information." 1>&2
  exit 1
fi
if test $# = 0 && test $mode = checkout; then
  echo "gitsub.sh: too few arguments in '$mode' mode" 1>&2
  echo "Try 'gitsub.sh --help' for more information." 1>&2
  exit 1
fi

# Read the configuration.
# Output:
# - subcheckout_names        space-separated list of subcheckout names
# - submodule_names          space-separated list of submodule names
if test -f .gitmodules; then
  subcheckout_names=`git config --file .gitmodules --get-regexp --name-only 'subcheckout\..*\.url' | sed -e 's/^subcheckout\.//' -e 's/\.url$//' | tr -d '\r' | tr '\n' ' '`
  submodule_names=`git config --file .gitmodules --get-regexp --name-only 'submodule\..*\.url' | sed -e 's/^submodule\.//' -e 's/\.url$//' | tr -d '\r' | tr '\n' ' '`
else
  subcheckout_names=
  submodule_names=
fi

# func_validate SUBDIR
# Verifies that the state on the file system is in sync with the declarations
# in the configuration file.
# Input:
# - subcheckout_names        space-separated list of subcheckout names
# - submodule_names          space-separated list of submodule names
# Output:
# - srcdirvar                Environment that the user can set
# - srcdir                   Value of the environment variable
# - path                     if $srcdir = "": relative path of the subdirectory
# - needs_init               if $srcdir = "" and $path is not yet initialized:
#                            true
# - url                      if $srcdir = "" and $path is not yet initialized:
#                            the repository URL
func_validate ()
{
  srcdirvar=`echo "$1" | LC_ALL=C sed -e 's/[^a-zA-Z0-9]/_/g' | LC_ALL=C tr '[a-z]' '[A-Z]'`"_SRCDIR"
  eval 'srcdir=$'"$srcdirvar"
  path=
  url=
  if test -n "$srcdir"; then
    func_note "Ignoring '$1' because $srcdirvar is set."
  else
    found=false
    needs_init=
    case " $subcheckout_names " in *" $1 "*)
      found=true
      # It ought to be a subcheckout.
      path=`git config --file .gitmodules "subcheckout.$1.path"`
      if test -z "$path"; then
        path="$1"
      fi
      if test -d "$path"; then
        if test -d "$path/.git"; then
          # It's a plain checkout.
          :
        else
          if test -f "$path/.git"; then
            # It's a submodule.
            func_fatal_error "Subdirectory '$path' is supposed to be a plain checkout, but it is a submodule."
          else
            func_warning "Ignoring '$path' because it exists but is not a git checkout."
          fi
        fi
      else
        # The subdir does not yet exist.
        needs_init=true
        url=`git config --file .gitmodules "subcheckout.$1.url"`
        if test -z "$url"; then
          func_fatal_error "Property subcheckout.$1.url is not defined in .gitmodules"
        fi
      fi
      ;;
    esac
    case " $submodule_names " in *" $1 "*)
      found=true
      # It ought to be a submodule.
      path=`git config --file .gitmodules "submodule.$1.path"`
      if test -z "$path"; then
        path="$1"
      fi
      if test -d "$path"; then
        if test -d "$path/.git" || test -f "$path/.git"; then
          # It's likely a submodule.
          :
        else
          path_if_empty=`find "$path" -prune -empty 2>/dev/null`
          if test -n "$path_if_empty"; then
            # The subdir is empty.
            needs_init=true
          else
            # The subdir is not empty.
            # It is important to report an error, because we don't want to erase
            # the user's files and 'git submodule update gnulib' sometimes reports
            # "fatal: destination path '$path' already exists and is not an empty directory."
            # but sometimes does not.
            func_fatal_error "Subdir '$path' exists but is not a git checkout."
          fi
        fi
      else
        # The subdir does not yet exist.
        needs_init=true
      fi
      # Another way to determine needs_init could be:
      # if git submodule status "$path" | grep '^-' > /dev/null; then
      #   needs_init=true
      # fi
      if test -n "$needs_init"; then
        url=`git config --file .gitmodules "submodule.$1.url"`
        if test -z "$url"; then
          func_fatal_error "Property submodule.$1.url is not defined in .gitmodules"
        fi
      fi
      ;;
    esac
    if ! $found; then
      func_fatal_error "Subdir '$1' is not configured as a subcheckout or a submodule in .gitmodules"
    fi
  fi
}

# func_cleanup_current_git_clone
# Cleans up the current 'git clone' operation.
# Input:
# - path
func_cleanup_current_git_clone ()
{
  rm -rf "$path"
  func_fatal_error "git clone failed"
}

# func_pull SUBDIR GIT_OPTIONS
# Implements the 'pull' operation.
func_pull ()
{
  func_validate "$1"
  if test -z "$srcdir"; then
    case " $subcheckout_names " in *" $1 "*)
      # It's a subcheckout.
      if test -d "$path"; then
        if test -d "$path/.git"; then
          (cd "$path" && git pull) || func_fatal_error "git operation failed"
        fi
      else
        # The subdir does not yet exist. Create a plain checkout.
        trap func_cleanup_current_git_clone 1 2 13 15
        git clone $2 "$url" "$path" || func_cleanup_current_git_clone
        trap - 1 2 13 15
      fi
      ;;
    esac
    case " $submodule_names " in *" $1 "*)
      # It's a submodule.
      if test -n "$needs_init"; then
        # Create a submodule checkout.
        git submodule init -- "$path" && git submodule update $2 -- "$path" || func_fatal_error "git operation failed"
      else
        # See https://stackoverflow.com/questions/1030169/easy-way-to-pull-latest-of-all-git-submodules
        # https://stackoverflow.com/questions/4611512/is-there-a-way-to-make-git-pull-automatically-update-submodules
        git submodule update "$path" || func_fatal_error "git operation failed"
      fi
      ;;
    esac
  fi
}

# func_upgrade SUBDIR
# Implements the 'upgrade' operation.
func_upgrade ()
{
  func_validate "$1"
  if test -z "$srcdir"; then
    if test -d "$path"; then
      case " $subcheckout_names " in *" $1 "*)
        # It's a subcheckout.
        if test -d "$path/.git"; then
          (cd "$path" && git pull) || func_fatal_error "git operation failed"
        fi
        ;;
      esac
      case " $submodule_names " in *" $1 "*)
        # It's a submodule.
        if test -z "$needs_init"; then
          (cd "$path" && git fetch && git merge origin/master) || func_fatal_error "git operation failed"
        fi
        ;;
      esac
    else
      # The subdir does not yet exist.
      func_fatal_error "Subdirectory '$path' does not exist yet. Use 'gitsub.sh pull' to create it."
    fi
  fi
}

# func_checkout SUBDIR REVISION
# Implements the 'checkout' operation.
func_checkout ()
{
  func_validate "$1"
  if test -z "$srcdir"; then
    if test -d "$path"; then
      case " $subcheckout_names " in *" $1 "*)
        # It's a subcheckout.
        if test -d "$path/.git"; then
          (cd "$path" && git checkout "$2") || func_fatal_error "git operation failed"
        fi
        ;;
      esac
      case " $submodule_names " in *" $1 "*)
        # It's a submodule.
        if test -z "$needs_init"; then
          (cd "$path" && git checkout "$2") || func_fatal_error "git operation failed"
        fi
        ;;
      esac
    else
      # The subdir does not yet exist.
      func_fatal_error "Subdirectory '$path' does not exist yet. Use 'gitsub.sh pull' to create it."
    fi
  fi
}

case "$mode" in
  pull )
    git_options=""
    while test $# -gt 0; do
      case "$1" in
        --reference=* | --depth=* | --recursive)
          git_options="$git_options $1"
          shift
          ;;
        --reference | --depth)
          git_options="$git_options $1 $2"
          shift; shift
          ;;
        *)
          break
          ;;
      esac
    done
    if test $# -gt 1; then
      echo "gitsub.sh: too many arguments in '$mode' mode" 1>&2
      echo "Try 'gitsub.sh --help' for more information." 1>&2
      exit 1
    fi
    if test $# = 0; then
      for sub in $subcheckout_names $submodule_names; do
        func_pull "$sub" "$git_options"
      done
    else
      valid=false
      for sub in $subcheckout_names $submodule_names; do
        if test "$sub" = "$1"; then
          valid=true
        fi
      done
      if $valid; then
        func_pull "$1" "$git_options"
      else
        func_fatal_error "Subdir '$1' is not configured as a subcheckout or a submodule in .gitmodules"
      fi
    fi
    ;;

  upgrade )
    if test $# = 0; then
      for sub in $subcheckout_names $submodule_names; do
        func_upgrade "$sub"
      done
    else
      valid=false
      for sub in $subcheckout_names $submodule_names; do
        if test "$sub" = "$1"; then
          valid=true
        fi
      done
      if $valid; then
        func_upgrade "$1"
      else
        func_fatal_error "Subdir '$1' is not configured as a subcheckout or a submodule in .gitmodules"
      fi
    fi
    ;;

  checkout )
    valid=false
    for sub in $subcheckout_names $submodule_names; do
      if test "$sub" = "$1"; then
        valid=true
      fi
    done
    if $valid; then
      func_checkout "$1" "$2"
    else
      func_fatal_error "Subdir '$1' is not configured as a subcheckout or a submodule in .gitmodules"
    fi
    ;;
esac
