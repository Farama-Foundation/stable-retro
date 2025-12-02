#! /bin/sh
# Test suite for argp.
# Copyright (C) 2006-2021 Free Software Foundation, Inc.
# This file is part of the GNUlib Library.
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

. "${srcdir=.}/init.sh"; path_prepend_ .

ERR=0

unset ARGP_HELP_FMT

func_compare() {
# If argp was compiled without base_name, it will display full program name.
# If run on mingw, it will display the program name with a .exe suffix.
  sed '1{
         s,: [^ ]*/test-argp,: test-argp,
         s,: test-argp\.exe,: test-argp,
        }' | LC_ALL=C tr -d '\r' > out
  compare expected out
}

####
# Test --usage output
cat > expected <<EOT
Usage: test-argp [-tvCSOlp?V] [-f FILE] [-r FILE] [-o[ARG]] [--test]
            [--file=FILE] [--input=FILE] [--read=FILE] [--verbose] [--cantiga]
            [--sonet] [--option] [--optional[=ARG]] [--many] [--one] [--two]
            [--limerick] [--poem] [--help] [--usage] [--version] ARGS...
EOT

${CHECKER} test-argp${EXEEXT} --usage | func_compare || ERR=1

####
# Test working usage-indent format

cat > expected <<EOT
Usage: test-argp [-tvCSOlp?V] [-f FILE] [-r FILE] [-o[ARG]] [--test]
[--file=FILE] [--input=FILE] [--read=FILE] [--verbose] [--cantiga] [--sonet]
[--option] [--optional[=ARG]] [--many] [--one] [--two] [--limerick] [--poem]
[--help] [--usage] [--version] ARGS...
EOT

ARGP_HELP_FMT='usage-indent=0' ${CHECKER} test-argp${EXEEXT} --usage | func_compare || ERR=1

####
# Test --help output
cat >expected <<EOT
Usage: test-argp [OPTION...] ARGS...
documentation string

 Main options
  -t, --test

 Option Group 1
  -f, -r, --file=FILE, --input=FILE, --read=FILE
                             Option with a mandatory argument
  -v, --verbose              Simple option without arguments

 Option Group 1.1
  -C, --cantiga              create a cantiga
  -S, --sonet                create a sonet

 Option Group 2
  -O, --option               An option

  -o, --optional[=ARG]       Option with an optional argument. ARG is one of
                             the following:

  many                       many units
  one                        one unit
  two                        two units

 Option Group 2.1
  -l, --limerick             create a limerick
  -p, --poem                 create a poem

  -?, --help                 give this help list
      --usage                give a short usage message
  -V, --version              print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to <>.
EOT

# Compare --help output, but filter out any bug-reporting email address.
${CHECKER} test-argp${EXEEXT} --help \
    | sed 's/^\(Report bugs to \)<[^>]*>.$/\1<>./' | func_compare || ERR=1

####
# Test ambiguous option handling

${CHECKER} test-argp${EXEEXT} --optio 2>/dev/null && ERR=1

####
# Run built-in tests
${CHECKER} test-argp${EXEEXT} || ERR=1

Exit $ERR
