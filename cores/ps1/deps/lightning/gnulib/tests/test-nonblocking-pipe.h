/* Test for nonblocking read and write.

   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

/* A data block ought to be larger than the size of the in-kernel buffer.
   Working values of PIPE_DATA_BLOCK_SIZE, depending on kernel:

     Platform                        PIPE_DATA_BLOCK_SIZE

     Linux                           >= 63489
     Linux/SPARC                     >= 126977
     Linux/IA-64, Linux/MIPS         >= 253953
     Linux/PPC64                     >= 1048576
     FreeBSD, OpenBSD, Mac OS X      >= 65537
     AIX                             >= 32769
     HP-UX                           >= 8193
     IRIX                            >= 10241
     OSF/1                           >= 262145
     Solaris <= 7                    >= 10241
     Solaris >= 8                    >= 20481
     z/OS                            >= 131073
     Cygwin                          >= 65537
     native Windows                  >= 4097 (depends on the _pipe argument)
 */
#if defined __MVS__ || defined __osf__ || (defined __linux__ && (defined __ia64__ || defined __mips__))
# define PIPE_DATA_BLOCK_SIZE 270000
#elif defined __linux__ && defined __sparc__
# define PIPE_DATA_BLOCK_SIZE 140000
#elif defined __linux__ && defined __powerpc__
# define PIPE_DATA_BLOCK_SIZE 1100000
#elif defined _WIN32 && !defined __CYGWIN__
# define PIPE_DATA_BLOCK_SIZE 10000
#else
# define PIPE_DATA_BLOCK_SIZE 70000
#endif
