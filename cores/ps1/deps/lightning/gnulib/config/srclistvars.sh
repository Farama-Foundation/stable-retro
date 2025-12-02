# Variables for srclist-update and srclist.txt.
# Will likely change for each user.

# Copyright 2002-2021 Free Software Foundation, Inc.

# This file is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
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

# per-user overrides.
case $LOGNAME in
karl)
  : ${GNUORG=$HOME/gnu/gnuorg}
  : ${GNUWWWLICENSES=$HOME/gnu/www/www/licenses}
  ;;
esac

# Default to sibling (of parent) directories.
: ${AUTOCONF=../autoconf}
: ${AUTOMAKE=../automake}
: ${GMP=../gmp} # https://gmplib.org/devel/repo-usage -> hg pull -u
: ${GNUCONFIG=../config}
: ${GNULIBSRC=../gnulib}
: ${GNUORG=../gnuorg}
: ${GNUSTANDARDS=../gnustandards}
: ${GNUWWWLICENSES=../www/www/licenses}
: ${LIBCSRC=../libc}
: ${LIBGCRYPT=../libgcrypt} # https://dev.gnupg.org/source/libgcrypt.git
: ${LIBTOOL=../libtool}
: ${TEXINFOTEX=../../ftp.gnu.org/gnu/texinfo}
