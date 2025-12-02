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
   Working values of SOCKET_DATA_BLOCK_SIZE, depending on kernel:

     Platform                        SOCKET_DATA_BLOCK_SIZE

     Linux                           >= 7350000 (depends on circumstances)
     Linux/Android                   >= 1700000 (approx.)
     FreeBSD                         >= 107521
     OpenBSD                         >= 106430 (depends on circumstances)
     Mac OS X                        >= 680000 (depends on circumstances)
     AIX 5.1                         >= 125713
     AIX 7.1                         >= 200000 (depends on circumstances)
     HP-UX                           >= 114689
     IRIX                            >= 61089
     OSF/1                           >= 122881
     Solaris 7                       >= 63000 (depends on circumstances)
     Solaris 8                       >= 49153
     Solaris 9                       >= 73729
     Solaris 10                      >= 98305
     Solaris 11 2010-11              >= 73729
     Cygwin 1.5.x                    >= 66294401 but then write() fails with ENOBUFS
     Cygwin 1.7.x                    >= 163838 (depends on circumstances)
     native Windows                  >= 66294401
 */
#define SOCKET_DATA_BLOCK_SIZE 1000000

/* On Linux, Mac OS X, Cygwin 1.5.x, native Windows,
   sockets have very large buffers in the kernel, so that write() calls
   succeed before the reader has started reading, even if fd is blocking
   and the amount of data is larger than 1 MB.  */
#if defined __linux__ || defined __ANDROID__ || (defined __APPLE__ && defined __MACH__) || defined _WIN32 || defined __CYGWIN__
# define SOCKET_HAS_LARGE_BUFFER 1
#else
# define SOCKET_HAS_LARGE_BUFFER 0
#endif
