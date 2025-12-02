/* Test of strerrorname_np() function.

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
SIGNATURE_CHECK (strerrorname_np, const char *, (int));

#include <errno.h>

#include "macros.h"

int
main (void)
{
  /* Error codes specified by ISO C.  */
  ASSERT (strcmp (strerrorname_np (EDOM), "EDOM") == 0);
  ASSERT (strcmp (strerrorname_np (EILSEQ), "EILSEQ") == 0);
  ASSERT (strcmp (strerrorname_np (ERANGE), "ERANGE") == 0);

  /* Error codes specified by POSIX.
     <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html>  */
  #if defined E2BIG
  ASSERT (strcmp (strerrorname_np (E2BIG), "E2BIG") == 0);
  #endif
  #if defined EACCES
  ASSERT (strcmp (strerrorname_np (EACCES), "EACCES") == 0);
  #endif
  #if defined EADDRINUSE
  ASSERT (strcmp (strerrorname_np (EADDRINUSE), "EADDRINUSE") == 0);
  #endif
  #if defined EADDRNOTAVAIL
  ASSERT (strcmp (strerrorname_np (EADDRNOTAVAIL), "EADDRNOTAVAIL") == 0);
  #endif
  #if defined EAFNOSUPPORT
  ASSERT (strcmp (strerrorname_np (EAFNOSUPPORT), "EAFNOSUPPORT") == 0);
  #endif
  #if defined EAGAIN
  ASSERT (strcmp (strerrorname_np (EAGAIN), "EAGAIN") == 0);
  #endif
  #if defined EALREADY
  ASSERT (strcmp (strerrorname_np (EALREADY), "EALREADY") == 0);
  #endif
  #if defined EBADF
  ASSERT (strcmp (strerrorname_np (EBADF), "EBADF") == 0);
  #endif
  #if defined EBADMSG
  ASSERT (strcmp (strerrorname_np (EBADMSG), "EBADMSG") == 0);
  #endif
  #if defined EBUSY
  ASSERT (strcmp (strerrorname_np (EBUSY), "EBUSY") == 0);
  #endif
  #if defined ECANCELED
  ASSERT (strcmp (strerrorname_np (ECANCELED), "ECANCELED") == 0);
  #endif
  #if defined ECHILD
  ASSERT (strcmp (strerrorname_np (ECHILD), "ECHILD") == 0);
  #endif
  #if defined ECONNABORTED
  ASSERT (strcmp (strerrorname_np (ECONNABORTED), "ECONNABORTED") == 0);
  #endif
  #if defined ECONNREFUSED
  ASSERT (strcmp (strerrorname_np (ECONNREFUSED), "ECONNREFUSED") == 0);
  #endif
  #if defined ECONNRESET
  ASSERT (strcmp (strerrorname_np (ECONNRESET), "ECONNRESET") == 0);
  #endif
  #if defined EDEADLK
  ASSERT (strcmp (strerrorname_np (EDEADLK), "EDEADLK") == 0);
  #endif
  #if defined EDESTADDRREQ
  ASSERT (strcmp (strerrorname_np (EDESTADDRREQ), "EDESTADDRREQ") == 0);
  #endif
  #if defined EDQUOT
  ASSERT (strcmp (strerrorname_np (EDQUOT), "EDQUOT") == 0);
  #endif
  #if defined EEXIST
  ASSERT (strcmp (strerrorname_np (EEXIST), "EEXIST") == 0);
  #endif
  #if defined EFAULT
  ASSERT (strcmp (strerrorname_np (EFAULT), "EFAULT") == 0);
  #endif
  #if defined EFBIG
  ASSERT (strcmp (strerrorname_np (EFBIG), "EFBIG") == 0);
  #endif
  #if defined EHOSTUNREACH
  ASSERT (strcmp (strerrorname_np (EHOSTUNREACH), "EHOSTUNREACH") == 0);
  #endif
  #if defined EIDRM
  ASSERT (strcmp (strerrorname_np (EIDRM), "EIDRM") == 0);
  #endif
  #if defined EINPROGRESS
  ASSERT (strcmp (strerrorname_np (EINPROGRESS), "EINPROGRESS") == 0);
  #endif
  #if defined EINTR
  ASSERT (strcmp (strerrorname_np (EINTR), "EINTR") == 0);
  #endif
  #if defined EINVAL
  ASSERT (strcmp (strerrorname_np (EINVAL), "EINVAL") == 0);
  #endif
  #if defined EIO
  ASSERT (strcmp (strerrorname_np (EIO), "EIO") == 0);
  #endif
  #if defined EISCONN
  ASSERT (strcmp (strerrorname_np (EISCONN), "EISCONN") == 0);
  #endif
  #if defined EISDIR
  ASSERT (strcmp (strerrorname_np (EISDIR), "EISDIR") == 0);
  #endif
  #if defined ELOOP
  ASSERT (strcmp (strerrorname_np (ELOOP), "ELOOP") == 0);
  #endif
  #if defined EMFILE
  ASSERT (strcmp (strerrorname_np (EMFILE), "EMFILE") == 0);
  #endif
  #if defined EMLINK
  ASSERT (strcmp (strerrorname_np (EMLINK), "EMLINK") == 0);
  #endif
  #if defined EMSGSIZE
  ASSERT (strcmp (strerrorname_np (EMSGSIZE), "EMSGSIZE") == 0);
  #endif
  #if defined EMULTIHOP
  ASSERT (strcmp (strerrorname_np (EMULTIHOP), "EMULTIHOP") == 0);
  #endif
  #if defined ENAMETOOLONG
  ASSERT (strcmp (strerrorname_np (ENAMETOOLONG), "ENAMETOOLONG") == 0);
  #endif
  #if defined ENETDOWN
  ASSERT (strcmp (strerrorname_np (ENETDOWN), "ENETDOWN") == 0);
  #endif
  #if defined ENETRESET
  ASSERT (strcmp (strerrorname_np (ENETRESET), "ENETRESET") == 0);
  #endif
  #if defined ENETUNREACH
  ASSERT (strcmp (strerrorname_np (ENETUNREACH), "ENETUNREACH") == 0);
  #endif
  #if defined ENFILE
  ASSERT (strcmp (strerrorname_np (ENFILE), "ENFILE") == 0);
  #endif
  #if defined ENOBUFS
  ASSERT (strcmp (strerrorname_np (ENOBUFS), "ENOBUFS") == 0);
  #endif
  #if defined ENODATA
  ASSERT (strcmp (strerrorname_np (ENODATA), "ENODATA") == 0);
  #endif
  #if defined ENODEV
  ASSERT (strcmp (strerrorname_np (ENODEV), "ENODEV") == 0);
  #endif
  #if defined ENOENT
  ASSERT (strcmp (strerrorname_np (ENOENT), "ENOENT") == 0);
  #endif
  #if defined ENOEXEC
  ASSERT (strcmp (strerrorname_np (ENOEXEC), "ENOEXEC") == 0);
  #endif
  #if defined ENOLCK
  ASSERT (strcmp (strerrorname_np (ENOLCK), "ENOLCK") == 0);
  #endif
  #if defined ENOLINK
  ASSERT (strcmp (strerrorname_np (ENOLINK), "ENOLINK") == 0);
  #endif
  #if defined ENOMEM
  ASSERT (strcmp (strerrorname_np (ENOMEM), "ENOMEM") == 0);
  #endif
  #if defined ENOMSG
  ASSERT (strcmp (strerrorname_np (ENOMSG), "ENOMSG") == 0);
  #endif
  #if defined ENOPROTOOPT
  ASSERT (strcmp (strerrorname_np (ENOPROTOOPT), "ENOPROTOOPT") == 0);
  #endif
  #if defined ENOSPC
  ASSERT (strcmp (strerrorname_np (ENOSPC), "ENOSPC") == 0);
  #endif
  #if defined ENOSR
  ASSERT (strcmp (strerrorname_np (ENOSR), "ENOSR") == 0);
  #endif
  #if defined ENOSTR
  ASSERT (strcmp (strerrorname_np (ENOSTR), "ENOSTR") == 0);
  #endif
  #if defined ENOSYS
  ASSERT (strcmp (strerrorname_np (ENOSYS), "ENOSYS") == 0);
  #endif
  #if defined ENOTCONN
  ASSERT (strcmp (strerrorname_np (ENOTCONN), "ENOTCONN") == 0);
  #endif
  #if defined ENOTDIR
  ASSERT (strcmp (strerrorname_np (ENOTDIR), "ENOTDIR") == 0);
  #endif
  #if defined ENOTEMPTY && ENOTEMPTY != EEXIST
  ASSERT (strcmp (strerrorname_np (ENOTEMPTY), "ENOTEMPTY") == 0);
  #endif
  #if defined ENOTRECOVERABLE
  ASSERT (strcmp (strerrorname_np (ENOTRECOVERABLE), "ENOTRECOVERABLE") == 0);
  #endif
  #if defined ENOTSOCK
  ASSERT (strcmp (strerrorname_np (ENOTSOCK), "ENOTSOCK") == 0);
  #endif
  #if defined ENOTSUP && ENOTSUP != EOPNOTSUPP
  ASSERT (strcmp (strerrorname_np (ENOTSUP), "ENOTSUP") == 0);
  #endif
  #if defined ENOTTY
  ASSERT (strcmp (strerrorname_np (ENOTTY), "ENOTTY") == 0);
  #endif
  #if defined ENXIO
  ASSERT (strcmp (strerrorname_np (ENXIO), "ENXIO") == 0);
  #endif
  #if defined EOPNOTSUPP
  ASSERT (strcmp (strerrorname_np (EOPNOTSUPP), "EOPNOTSUPP") == 0);
  #endif
  #if defined EOVERFLOW
  ASSERT (strcmp (strerrorname_np (EOVERFLOW), "EOVERFLOW") == 0);
  #endif
  #if defined EOWNERDEAD
  ASSERT (strcmp (strerrorname_np (EOWNERDEAD), "EOWNERDEAD") == 0);
  #endif
  #if defined EPERM
  ASSERT (strcmp (strerrorname_np (EPERM), "EPERM") == 0);
  #endif
  #if defined EPIPE
  ASSERT (strcmp (strerrorname_np (EPIPE), "EPIPE") == 0);
  #endif
  #if defined EPROTO
  ASSERT (strcmp (strerrorname_np (EPROTO), "EPROTO") == 0);
  #endif
  #if defined EPROTONOSUPPORT
  ASSERT (strcmp (strerrorname_np (EPROTONOSUPPORT), "EPROTONOSUPPORT") == 0);
  #endif
  #if defined EPROTOTYPE
  ASSERT (strcmp (strerrorname_np (EPROTOTYPE), "EPROTOTYPE") == 0);
  #endif
  #if defined EROFS
  ASSERT (strcmp (strerrorname_np (EROFS), "EROFS") == 0);
  #endif
  #if defined ESPIPE
  ASSERT (strcmp (strerrorname_np (ESPIPE), "ESPIPE") == 0);
  #endif
  #if defined ESRCH
  ASSERT (strcmp (strerrorname_np (ESRCH), "ESRCH") == 0);
  #endif
  #if defined ESTALE
  ASSERT (strcmp (strerrorname_np (ESTALE), "ESTALE") == 0);
  #endif
  #if defined ETIME
  ASSERT (strcmp (strerrorname_np (ETIME), "ETIME") == 0);
  #endif
  #if defined ETIMEDOUT
  ASSERT (strcmp (strerrorname_np (ETIMEDOUT), "ETIMEDOUT") == 0);
  #endif
  #if defined ETXTBSY
  ASSERT (strcmp (strerrorname_np (ETXTBSY), "ETXTBSY") == 0);
  #endif
  #if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
  ASSERT (strcmp (strerrorname_np (EWOULDBLOCK), "EWOULDBLOCK") == 0);
  #endif
  #if defined EXDEV
  ASSERT (strcmp (strerrorname_np (EXDEV), "EXDEV") == 0);
  #endif

  /* Other error codes on other systems.  */
  /* Solaris */
  #if defined EADI
  ASSERT (strcmp (strerrorname_np (EADI), "EADI") == 0);
  #endif
  /* Linux, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EADV
  ASSERT (strcmp (strerrorname_np (EADV), "EADV") == 0);
  #endif
  /* OSF/1 */
  #if defined EAIO
  ASSERT (strcmp (strerrorname_np (EAIO), "EAIO") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix */
  #if defined EAUTH
  ASSERT (strcmp (strerrorname_np (EAUTH), "EAUTH") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EBACKGROUND
  ASSERT (strcmp (strerrorname_np (EBACKGROUND), "EBACKGROUND") == 0);
  #endif
  /* Mac OS X */
  #if defined EBADARCH
  ASSERT (strcmp (strerrorname_np (EBADARCH), "EBADARCH") == 0);
  #endif
  /* Minix */
  #if defined EBADCALL
  ASSERT (strcmp (strerrorname_np (EBADCALL), "EBADCALL") == 0);
  #endif
  /* Minix */
  #if defined EBADCPU
  ASSERT (strcmp (strerrorname_np (EBADCPU), "EBADCPU") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBADE
  ASSERT (strcmp (strerrorname_np (EBADE), "EBADE") == 0);
  #endif
  /* Minix */
  #if defined EBADEPT
  ASSERT (strcmp (strerrorname_np (EBADEPT), "EBADEPT") == 0);
  #endif
  /* Mac OS X */
  #if defined EBADEXEC
  ASSERT (strcmp (strerrorname_np (EBADEXEC), "EBADEXEC") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBADFD
  ASSERT (strcmp (strerrorname_np (EBADFD), "EBADFD") == 0);
  #endif
  /* IRIX */
  #if defined EBADFILT
  ASSERT (strcmp (strerrorname_np (EBADFILT), "EBADFILT") == 0);
  #endif
  /* Minix */
  #if defined EBADIOCTL
  ASSERT (strcmp (strerrorname_np (EBADIOCTL), "EBADIOCTL") == 0);
  #endif
  /* Mac OS X */
  #if defined EBADMACHO
  ASSERT (strcmp (strerrorname_np (EBADMACHO), "EBADMACHO") == 0);
  #endif
  /* Minix */
  #if defined EBADMODE
  ASSERT (strcmp (strerrorname_np (EBADMODE), "EBADMODE") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBADR
  ASSERT (strcmp (strerrorname_np (EBADR), "EBADR") == 0);
  #endif
  /* Minix */
  #if defined EBADREQUEST
  ASSERT (strcmp (strerrorname_np (EBADREQUEST), "EBADREQUEST") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix */
  #if defined EBADRPC
  ASSERT (strcmp (strerrorname_np (EBADRPC), "EBADRPC") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBADRQC
  ASSERT (strcmp (strerrorname_np (EBADRQC), "EBADRQC") == 0);
  #endif
  /* IRIX */
  #if defined EBADRSPEC
  ASSERT (strcmp (strerrorname_np (EBADRSPEC), "EBADRSPEC") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBADSLT
  ASSERT (strcmp (strerrorname_np (EBADSLT), "EBADSLT") == 0);
  #endif
  /* IRIX */
  #if defined EBADTSPEC
  ASSERT (strcmp (strerrorname_np (EBADTSPEC), "EBADTSPEC") == 0);
  #endif
  /* HP-UX */
  #if defined EBADVER
  ASSERT (strcmp (strerrorname_np (EBADVER), "EBADVER") == 0);
  #endif
  /* IRIX */
  #if defined EBDHDL
  ASSERT (strcmp (strerrorname_np (EBDHDL), "EBDHDL") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EBFONT
  ASSERT (strcmp (strerrorname_np (EBFONT), "EBFONT") == 0);
  #endif
  /* IRIX */
  #if defined EBUFSIZE
  ASSERT (strcmp (strerrorname_np (EBUFSIZE), "EBUFSIZE") == 0);
  #endif
  /* Minix */
  #if defined ECALLDENIED
  ASSERT (strcmp (strerrorname_np (ECALLDENIED), "ECALLDENIED") == 0);
  #endif
  /* IRIX */
  #if defined ECANTEXTENT
  ASSERT (strcmp (strerrorname_np (ECANTEXTENT), "ECANTEXTENT") == 0);
  #endif
  /* FreeBSD */
  #if defined ECAPMODE
  ASSERT (strcmp (strerrorname_np (ECAPMODE), "ECAPMODE") == 0);
  #endif
  /* Cygwin */
  #if defined ECASECLASH
  ASSERT (strcmp (strerrorname_np (ECASECLASH), "ECASECLASH") == 0);
  #endif
  /* IRIX */
  #if defined ECELLDOWN
  ASSERT (strcmp (strerrorname_np (ECELLDOWN), "ECELLDOWN") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ECHRNG
  ASSERT (strcmp (strerrorname_np (ECHRNG), "ECHRNG") == 0);
  #endif
  /* IRIX */
  #if defined ECKPT
  ASSERT (strcmp (strerrorname_np (ECKPT), "ECKPT") == 0);
  #endif
  /* IRIX */
  #if defined ECLOCKCPU
  ASSERT (strcmp (strerrorname_np (ECLOCKCPU), "ECLOCKCPU") == 0);
  #endif
  /* OSF/1 */
  #if defined ECLONEME && ECLONEME != ERESTART
  ASSERT (strcmp (strerrorname_np (ECLONEME), "ECLONEME") == 0);
  #endif
  /* Linux, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ECOMM
  ASSERT (strcmp (strerrorname_np (ECOMM), "ECOMM") == 0);
  #endif
  /* HP-UX */
  #if defined ECONFIG
  ASSERT (strcmp (strerrorname_np (ECONFIG), "ECONFIG") == 0);
  #endif
  /* IRIX */
  #if defined ECONTROLLER
  ASSERT (strcmp (strerrorname_np (ECONTROLLER), "ECONTROLLER") == 0);
  #endif
  /* AIX */
  #if defined ECORRUPT
  ASSERT (strcmp (strerrorname_np (ECORRUPT), "ECORRUPT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED
  ASSERT (strcmp (strerrorname_np (ED), "ED") == 0);
  #endif
  /* Minix */
  #if defined EDEADEPT
  ASSERT (strcmp (strerrorname_np (EDEADEPT), "EDEADEPT") == 0);
  #endif
  /* IRIX, Solaris, Cygwin */
  #if defined EDEADLOCK && EDEADLOCK != EDEADLK
  ASSERT (strcmp (strerrorname_np (EDEADLOCK), "EDEADLOCK") == 0);
  #endif
  /* Minix */
  #if defined EDEADSRCDST
  ASSERT (strcmp (strerrorname_np (EDEADSRCDST), "EDEADSRCDST") == 0);
  #endif
  /* IRIX */
  #if defined EDELAY
  ASSERT (strcmp (strerrorname_np (EDELAY), "EDELAY") == 0);
  #endif
  /* IRIX */
  #if defined EDESTROYED
  ASSERT (strcmp (strerrorname_np (EDESTROYED), "EDESTROYED") == 0);
  #endif
  /* Mac OS X */
  #if defined EDEVERR
  ASSERT (strcmp (strerrorname_np (EDEVERR), "EDEVERR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EDIED
  ASSERT (strcmp (strerrorname_np (EDIED), "EDIED") == 0);
  #endif
  /* IRIX */
  #if defined EDIRCORRUPTED
  ASSERT (strcmp (strerrorname_np (EDIRCORRUPTED), "EDIRCORRUPTED") == 0);
  #endif
  /* FreeBSD */
  #if defined EDIRIOCTL
  ASSERT (strcmp (strerrorname_np (EDIRIOCTL), "EDIRIOCTL") == 0);
  #endif
  /* OSF/1 */
  #if defined EDIRTY
  ASSERT (strcmp (strerrorname_np (EDIRTY), "EDIRTY") == 0);
  #endif
  /* IRIX */
  #if defined EDISJOINT
  ASSERT (strcmp (strerrorname_np (EDISJOINT), "EDISJOINT") == 0);
  #endif
  /* AIX */
  #if defined EDIST
  ASSERT (strcmp (strerrorname_np (EDIST), "EDIST") == 0);
  #endif
  /* Minix */
  #if defined EDONTREPLY
  ASSERT (strcmp (strerrorname_np (EDONTREPLY), "EDONTREPLY") == 0);
  #endif
  /* FreeBSD */
  #if defined EDOOFUS
  ASSERT (strcmp (strerrorname_np (EDOOFUS), "EDOOFUS") == 0);
  #endif
  /* Linux, HP-UX, Cygwin */
  #if defined EDOTDOT
  ASSERT (strcmp (strerrorname_np (EDOTDOT), "EDOTDOT") == 0);
  #endif
  /* OSF/1 */
  #if defined EDUPPKG
  ASSERT (strcmp (strerrorname_np (EDUPPKG), "EDUPPKG") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_ALREADY_OPEN
  ASSERT (strcmp (strerrorname_np (ED_ALREADY_OPEN), "ED_ALREADY_OPEN") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_DEVICE_DOWN
  ASSERT (strcmp (strerrorname_np (ED_DEVICE_DOWN), "ED_DEVICE_DOWN") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_INVALID_OPERATION
  ASSERT (strcmp (strerrorname_np (ED_INVALID_OPERATION), "ED_INVALID_OPERATION") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_INVALID_RECNUM
  ASSERT (strcmp (strerrorname_np (ED_INVALID_RECNUM), "ED_INVALID_RECNUM") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_INVALID_SIZE
  ASSERT (strcmp (strerrorname_np (ED_INVALID_SIZE), "ED_INVALID_SIZE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_IO_ERROR
  ASSERT (strcmp (strerrorname_np (ED_IO_ERROR), "ED_IO_ERROR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_NO_MEMORY
  ASSERT (strcmp (strerrorname_np (ED_NO_MEMORY), "ED_NO_MEMORY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_NO_SUCH_DEVICE
  ASSERT (strcmp (strerrorname_np (ED_NO_SUCH_DEVICE), "ED_NO_SUCH_DEVICE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_READ_ONLY
  ASSERT (strcmp (strerrorname_np (ED_READ_ONLY), "ED_READ_ONLY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined ED_WOULD_BLOCK
  ASSERT (strcmp (strerrorname_np (ED_WOULD_BLOCK), "ED_WOULD_BLOCK") == 0);
  #endif
  /* IRIX */
  #if defined EEMPTY
  ASSERT (strcmp (strerrorname_np (EEMPTY), "EEMPTY") == 0);
  #endif
  /* OSF/1 */
  #if defined EEMULATE
  ASSERT (strcmp (strerrorname_np (EEMULATE), "EEMULATE") == 0);
  #endif
  /* IRIX */
  #if defined EENDOFMINOR
  ASSERT (strcmp (strerrorname_np (EENDOFMINOR), "EENDOFMINOR") == 0);
  #endif
  /* IRIX */
  #if defined EENQUEUED
  ASSERT (strcmp (strerrorname_np (EENQUEUED), "EENQUEUED") == 0);
  #endif
  /* OSF/1 */
  #if defined EFAIL
  ASSERT (strcmp (strerrorname_np (EFAIL), "EFAIL") == 0);
  #endif
  /* AIX */
  #if defined EFORMAT
  ASSERT (strcmp (strerrorname_np (EFORMAT), "EFORMAT") == 0);
  #endif
  /* Haiku */
  #if defined EFPOS
  ASSERT (strcmp (strerrorname_np (EFPOS), "EFPOS") == 0);
  #endif
  /* IRIX */
  #if defined EFSCORRUPTED
  ASSERT (strcmp (strerrorname_np (EFSCORRUPTED), "EFSCORRUPTED") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix, Cygwin */
  #if defined EFTYPE
  ASSERT (strcmp (strerrorname_np (EFTYPE), "EFTYPE") == 0);
  #endif
  /* Minix */
  #if defined EGENERIC
  ASSERT (strcmp (strerrorname_np (EGENERIC), "EGENERIC") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EGRATUITOUS
  ASSERT (strcmp (strerrorname_np (EGRATUITOUS), "EGRATUITOUS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EGREGIOUS
  ASSERT (strcmp (strerrorname_np (EGREGIOUS), "EGREGIOUS") == 0);
  #endif
  /* IRIX */
  #if defined EGROUPLOOP
  ASSERT (strcmp (strerrorname_np (EGROUPLOOP), "EGROUPLOOP") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Haiku, Cygwin */
  #if defined EHOSTDOWN
  ASSERT (strcmp (strerrorname_np (EHOSTDOWN), "EHOSTDOWN") == 0);
  #endif
  /* Linux */
  #if defined EHWPOISON
  ASSERT (strcmp (strerrorname_np (EHWPOISON), "EHWPOISON") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EIEIO
  ASSERT (strcmp (strerrorname_np (EIEIO), "EIEIO") == 0);
  #endif
  /* IRIX */
  #if defined EINIT
  ASSERT (strcmp (strerrorname_np (EINIT), "EINIT") == 0);
  #endif
  /* OSF/1 */
  #if defined EINPROG
  ASSERT (strcmp (strerrorname_np (EINPROG), "EINPROG") == 0);
  #endif
  /* IRIX */
  #if defined EINVALMODE
  ASSERT (strcmp (strerrorname_np (EINVALMODE), "EINVALMODE") == 0);
  #endif
  /* IRIX */
  #if defined EINVALSTATE
  ASSERT (strcmp (strerrorname_np (EINVALSTATE), "EINVALSTATE") == 0);
  #endif
  /* IRIX */
  #if defined EINVALTIME
  ASSERT (strcmp (strerrorname_np (EINVALTIME), "EINVALTIME") == 0);
  #endif
  /* IRIX */
  #if defined EIORESID
  ASSERT (strcmp (strerrorname_np (EIORESID), "EIORESID") == 0);
  #endif
  /* OpenBSD */
  #if defined EIPSEC
  ASSERT (strcmp (strerrorname_np (EIPSEC), "EIPSEC") == 0);
  #endif
  /* Linux, IRIX */
  #if defined EISNAM
  ASSERT (strcmp (strerrorname_np (EISNAM), "EISNAM") == 0);
  #endif
  /* IRIX */
  #if defined EJOINED
  ASSERT (strcmp (strerrorname_np (EJOINED), "EJOINED") == 0);
  #endif
  /* FreeBSD, OSF/1 */
  #if defined EJUSTRETURN
  ASSERT (strcmp (strerrorname_np (EJUSTRETURN), "EJUSTRETURN") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_ABORTED
  ASSERT (strcmp (strerrorname_np (EKERN_ABORTED), "EKERN_ABORTED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_FAILURE
  ASSERT (strcmp (strerrorname_np (EKERN_FAILURE), "EKERN_FAILURE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INTERRUPTED
  ASSERT (strcmp (strerrorname_np (EKERN_INTERRUPTED), "EKERN_INTERRUPTED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_ADDRESS
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_ADDRESS), "EKERN_INVALID_ADDRESS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_ARGUMENT
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_ARGUMENT), "EKERN_INVALID_ARGUMENT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_CAPABILITY
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_CAPABILITY), "EKERN_INVALID_CAPABILITY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_HOST
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_HOST), "EKERN_INVALID_HOST") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_NAME
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_NAME), "EKERN_INVALID_NAME") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_RIGHT
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_RIGHT), "EKERN_INVALID_RIGHT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_TASK
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_TASK), "EKERN_INVALID_TASK") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_INVALID_VALUE
  ASSERT (strcmp (strerrorname_np (EKERN_INVALID_VALUE), "EKERN_INVALID_VALUE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_MEMORY_ERROR
  ASSERT (strcmp (strerrorname_np (EKERN_MEMORY_ERROR), "EKERN_MEMORY_ERROR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_MEMORY_FAILURE
  ASSERT (strcmp (strerrorname_np (EKERN_MEMORY_FAILURE), "EKERN_MEMORY_FAILURE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_MEMORY_PRESENT
  ASSERT (strcmp (strerrorname_np (EKERN_MEMORY_PRESENT), "EKERN_MEMORY_PRESENT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_NAME_EXISTS
  ASSERT (strcmp (strerrorname_np (EKERN_NAME_EXISTS), "EKERN_NAME_EXISTS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_NOT_IN_SET
  ASSERT (strcmp (strerrorname_np (EKERN_NOT_IN_SET), "EKERN_NOT_IN_SET") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_NOT_RECEIVER
  ASSERT (strcmp (strerrorname_np (EKERN_NOT_RECEIVER), "EKERN_NOT_RECEIVER") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_NO_ACCESS
  ASSERT (strcmp (strerrorname_np (EKERN_NO_ACCESS), "EKERN_NO_ACCESS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_NO_SPACE
  ASSERT (strcmp (strerrorname_np (EKERN_NO_SPACE), "EKERN_NO_SPACE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_PROTECTION_FAILURE
  ASSERT (strcmp (strerrorname_np (EKERN_PROTECTION_FAILURE), "EKERN_PROTECTION_FAILURE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_RESOURCE_SHORTAGE
  ASSERT (strcmp (strerrorname_np (EKERN_RESOURCE_SHORTAGE), "EKERN_RESOURCE_SHORTAGE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_RIGHT_EXISTS
  ASSERT (strcmp (strerrorname_np (EKERN_RIGHT_EXISTS), "EKERN_RIGHT_EXISTS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_TERMINATED
  ASSERT (strcmp (strerrorname_np (EKERN_TERMINATED), "EKERN_TERMINATED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_TIMEDOUT
  ASSERT (strcmp (strerrorname_np (EKERN_TIMEDOUT), "EKERN_TIMEDOUT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_UREFS_OVERFLOW
  ASSERT (strcmp (strerrorname_np (EKERN_UREFS_OVERFLOW), "EKERN_UREFS_OVERFLOW") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EKERN_WRITE_PROTECTION_FAILURE
  ASSERT (strcmp (strerrorname_np (EKERN_WRITE_PROTECTION_FAILURE), "EKERN_WRITE_PROTECTION_FAILURE") == 0);
  #endif
  /* Linux */
  #if defined EKEYEXPIRED
  ASSERT (strcmp (strerrorname_np (EKEYEXPIRED), "EKEYEXPIRED") == 0);
  #endif
  /* Linux */
  #if defined EKEYREJECTED
  ASSERT (strcmp (strerrorname_np (EKEYREJECTED), "EKEYREJECTED") == 0);
  #endif
  /* Linux */
  #if defined EKEYREVOKED
  ASSERT (strcmp (strerrorname_np (EKEYREVOKED), "EKEYREVOKED") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EL2HLT
  ASSERT (strcmp (strerrorname_np (EL2HLT), "EL2HLT") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EL2NSYNC
  ASSERT (strcmp (strerrorname_np (EL2NSYNC), "EL2NSYNC") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EL3HLT
  ASSERT (strcmp (strerrorname_np (EL3HLT), "EL3HLT") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EL3RST
  ASSERT (strcmp (strerrorname_np (EL3RST), "EL3RST") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix */
  #if defined ELAST && 0
  ASSERT (strcmp (strerrorname_np (ELAST), "ELAST") == 0);
  #endif
  /* Cygwin */
  #if defined ELBIN
  ASSERT (strcmp (strerrorname_np (ELBIN), "ELBIN") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ELIBACC
  ASSERT (strcmp (strerrorname_np (ELIBACC), "ELIBACC") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ELIBBAD
  ASSERT (strcmp (strerrorname_np (ELIBBAD), "ELIBBAD") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ELIBEXEC
  ASSERT (strcmp (strerrorname_np (ELIBEXEC), "ELIBEXEC") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ELIBMAX
  ASSERT (strcmp (strerrorname_np (ELIBMAX), "ELIBMAX") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ELIBSCN
  ASSERT (strcmp (strerrorname_np (ELIBSCN), "ELIBSCN") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ELNRNG
  ASSERT (strcmp (strerrorname_np (ELNRNG), "ELNRNG") == 0);
  #endif
  /* Minix */
  #if defined ELOCKED
  ASSERT (strcmp (strerrorname_np (ELOCKED), "ELOCKED") == 0);
  #endif
  /* Solaris */
  #if defined ELOCKUNMAPPED
  ASSERT (strcmp (strerrorname_np (ELOCKUNMAPPED), "ELOCKUNMAPPED") == 0);
  #endif
  /* IRIX */
  #if defined ELOGINLIM
  ASSERT (strcmp (strerrorname_np (ELOGINLIM), "ELOGINLIM") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_BODY_ERROR
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_BODY_ERROR), "EMACH_RCV_BODY_ERROR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_HEADER_ERROR
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_HEADER_ERROR), "EMACH_RCV_HEADER_ERROR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_INTERRUPTED
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_INTERRUPTED), "EMACH_RCV_INTERRUPTED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_INVALID_DATA
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_INVALID_DATA), "EMACH_RCV_INVALID_DATA") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_INVALID_NAME
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_INVALID_NAME), "EMACH_RCV_INVALID_NAME") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_INVALID_NOTIFY
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_INVALID_NOTIFY), "EMACH_RCV_INVALID_NOTIFY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_IN_PROGRESS
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_IN_PROGRESS), "EMACH_RCV_IN_PROGRESS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_IN_SET
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_IN_SET), "EMACH_RCV_IN_SET") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_PORT_CHANGED
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_PORT_CHANGED), "EMACH_RCV_PORT_CHANGED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_PORT_DIED
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_PORT_DIED), "EMACH_RCV_PORT_DIED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_TIMED_OUT
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_TIMED_OUT), "EMACH_RCV_TIMED_OUT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_RCV_TOO_LARGE
  ASSERT (strcmp (strerrorname_np (EMACH_RCV_TOO_LARGE), "EMACH_RCV_TOO_LARGE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INTERRUPTED
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INTERRUPTED), "EMACH_SEND_INTERRUPTED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_DATA
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_DATA), "EMACH_SEND_INVALID_DATA") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_DEST
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_DEST), "EMACH_SEND_INVALID_DEST") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_HEADER
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_HEADER), "EMACH_SEND_INVALID_HEADER") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_MEMORY
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_MEMORY), "EMACH_SEND_INVALID_MEMORY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_NOTIFY
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_NOTIFY), "EMACH_SEND_INVALID_NOTIFY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_REPLY
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_REPLY), "EMACH_SEND_INVALID_REPLY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_RIGHT
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_RIGHT), "EMACH_SEND_INVALID_RIGHT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_INVALID_TYPE
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_INVALID_TYPE), "EMACH_SEND_INVALID_TYPE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_IN_PROGRESS
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_IN_PROGRESS), "EMACH_SEND_IN_PROGRESS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_MSG_TOO_SMALL
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_MSG_TOO_SMALL), "EMACH_SEND_MSG_TOO_SMALL") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_NOTIFY_IN_PROGRESS
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_NOTIFY_IN_PROGRESS), "EMACH_SEND_NOTIFY_IN_PROGRESS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_NO_BUFFER
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_NO_BUFFER), "EMACH_SEND_NO_BUFFER") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_NO_NOTIFY
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_NO_NOTIFY), "EMACH_SEND_NO_NOTIFY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_TIMED_OUT
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_TIMED_OUT), "EMACH_SEND_TIMED_OUT") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMACH_SEND_WILL_NOTIFY
  ASSERT (strcmp (strerrorname_np (EMACH_SEND_WILL_NOTIFY), "EMACH_SEND_WILL_NOTIFY") == 0);
  #endif
  /* AIX, OSF/1 */
  #if defined EMEDIA
  ASSERT (strcmp (strerrorname_np (EMEDIA), "EMEDIA") == 0);
  #endif
  /* Linux, OpenBSD */
  #if defined EMEDIUMTYPE
  ASSERT (strcmp (strerrorname_np (EMEDIUMTYPE), "EMEDIUMTYPE") == 0);
  #endif
  /* IRIX */
  #if defined EMEMRETRY
  ASSERT (strcmp (strerrorname_np (EMEMRETRY), "EMEMRETRY") == 0);
  #endif
  /* IRIX */
  #if defined EMIGRATED
  ASSERT (strcmp (strerrorname_np (EMIGRATED), "EMIGRATED") == 0);
  #endif
  /* IRIX */
  #if defined EMIGRATING
  ASSERT (strcmp (strerrorname_np (EMIGRATING), "EMIGRATING") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_ARRAY_TOO_LARGE
  ASSERT (strcmp (strerrorname_np (EMIG_ARRAY_TOO_LARGE), "EMIG_ARRAY_TOO_LARGE") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_BAD_ARGUMENTS
  ASSERT (strcmp (strerrorname_np (EMIG_BAD_ARGUMENTS), "EMIG_BAD_ARGUMENTS") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_BAD_ID
  ASSERT (strcmp (strerrorname_np (EMIG_BAD_ID), "EMIG_BAD_ID") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_DESTROY_REQUEST
  ASSERT (strcmp (strerrorname_np (EMIG_DESTROY_REQUEST), "EMIG_DESTROY_REQUEST") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_EXCEPTION
  ASSERT (strcmp (strerrorname_np (EMIG_EXCEPTION), "EMIG_EXCEPTION") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_NO_REPLY
  ASSERT (strcmp (strerrorname_np (EMIG_NO_REPLY), "EMIG_NO_REPLY") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_REMOTE_ERROR
  ASSERT (strcmp (strerrorname_np (EMIG_REMOTE_ERROR), "EMIG_REMOTE_ERROR") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_REPLY_MISMATCH
  ASSERT (strcmp (strerrorname_np (EMIG_REPLY_MISMATCH), "EMIG_REPLY_MISMATCH") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_SERVER_DIED
  ASSERT (strcmp (strerrorname_np (EMIG_SERVER_DIED), "EMIG_SERVER_DIED") == 0);
  #endif
  /* GNU/Hurd */
  #if defined EMIG_TYPE_ERROR
  ASSERT (strcmp (strerrorname_np (EMIG_TYPE_ERROR), "EMIG_TYPE_ERROR") == 0);
  #endif
  /* OSF/1 */
  #if defined EMTIMERS
  ASSERT (strcmp (strerrorname_np (EMTIMERS), "EMTIMERS") == 0);
  #endif
  /* IRIX */
  #if defined EMUSTRUN
  ASSERT (strcmp (strerrorname_np (EMUSTRUN), "EMUSTRUN") == 0);
  #endif
  /* Linux, IRIX */
  #if defined ENAVAIL
  ASSERT (strcmp (strerrorname_np (ENAVAIL), "ENAVAIL") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, Minix */
  #if defined ENEEDAUTH
  ASSERT (strcmp (strerrorname_np (ENEEDAUTH), "ENEEDAUTH") == 0);
  #endif
  /* IRIX */
  #if defined ENFSREMOTE
  ASSERT (strcmp (strerrorname_np (ENFSREMOTE), "ENFSREMOTE") == 0);
  #endif
  /* Cygwin */
  #if defined ENMFILE
  ASSERT (strcmp (strerrorname_np (ENMFILE), "ENMFILE") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ENOANO
  ASSERT (strcmp (strerrorname_np (ENOANO), "ENOANO") == 0);
  #endif
  /* IRIX */
  #if defined ENOATTACH
  ASSERT (strcmp (strerrorname_np (ENOATTACH), "ENOATTACH") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, IRIX, Minix */
  #if defined ENOATTR
  ASSERT (strcmp (strerrorname_np (ENOATTR), "ENOATTR") == 0);
  #endif
  /* IRIX */
  #if defined ENOBWD
  ASSERT (strcmp (strerrorname_np (ENOBWD), "ENOBWD") == 0);
  #endif
  /* MirBSD */
  #if defined ENOCOFFEE
  ASSERT (strcmp (strerrorname_np (ENOCOFFEE), "ENOCOFFEE") == 0);
  #endif
  /* Minix */
  #if defined ENOCONN
  ASSERT (strcmp (strerrorname_np (ENOCONN), "ENOCONN") == 0);
  #endif
  /* AIX */
  #if defined ENOCONNECT
  ASSERT (strcmp (strerrorname_np (ENOCONNECT), "ENOCONNECT") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ENOCSI
  ASSERT (strcmp (strerrorname_np (ENOCSI), "ENOCSI") == 0);
  #endif
  /* IRIX */
  #if defined ENOEXIST
  ASSERT (strcmp (strerrorname_np (ENOEXIST), "ENOEXIST") == 0);
  #endif
  /* IRIX */
  #if defined ENOINTRGROUP
  ASSERT (strcmp (strerrorname_np (ENOINTRGROUP), "ENOINTRGROUP") == 0);
  #endif
  /* FreeBSD */
  #if defined ENOIOCTL
  ASSERT (strcmp (strerrorname_np (ENOIOCTL), "ENOIOCTL") == 0);
  #endif
  /* Linux */
  #if defined ENOKEY
  ASSERT (strcmp (strerrorname_np (ENOKEY), "ENOKEY") == 0);
  #endif
  /* IRIX */
  #if defined ENOLIMFILE
  ASSERT (strcmp (strerrorname_np (ENOLIMFILE), "ENOLIMFILE") == 0);
  #endif
  /* HP-UX */
  #if defined ENOLOAD
  ASSERT (strcmp (strerrorname_np (ENOLOAD), "ENOLOAD") == 0);
  #endif
  /* IRIX */
  #if defined ENOLOGIN
  ASSERT (strcmp (strerrorname_np (ENOLOGIN), "ENOLOGIN") == 0);
  #endif
  /* HP-UX */
  #if defined ENOMATCH
  ASSERT (strcmp (strerrorname_np (ENOMATCH), "ENOMATCH") == 0);
  #endif
  /* Linux, OpenBSD, Cygwin */
  #if defined ENOMEDIUM
  ASSERT (strcmp (strerrorname_np (ENOMEDIUM), "ENOMEDIUM") == 0);
  #endif
  /* IRIX */
  #if defined ENOMESSAGE
  ASSERT (strcmp (strerrorname_np (ENOMESSAGE), "ENOMESSAGE") == 0);
  #endif
  /* Linux, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ENONET
  ASSERT (strcmp (strerrorname_np (ENONET), "ENONET") == 0);
  #endif
  /* Linux, HP-UX, IRIX, OSF/1, Solaris, Cygwin */
  #if defined ENOPKG
  ASSERT (strcmp (strerrorname_np (ENOPKG), "ENOPKG") == 0);
  #endif
  /* Mac OS X */
  #if defined ENOPOLICY
  ASSERT (strcmp (strerrorname_np (ENOPOLICY), "ENOPOLICY") == 0);
  #endif
  /* IRIX */
  #if defined ENOPROC
  ASSERT (strcmp (strerrorname_np (ENOPROC), "ENOPROC") == 0);
  #endif
  /* HP-UX */
  #if defined ENOREG
  ASSERT (strcmp (strerrorname_np (ENOREG), "ENOREG") == 0);
  #endif
  /* IRIX */
  #if defined ENOSERVICE
  ASSERT (strcmp (strerrorname_np (ENOSERVICE), "ENOSERVICE") == 0);
  #endif
  /* Cygwin */
  #if defined ENOSHARE
  ASSERT (strcmp (strerrorname_np (ENOSHARE), "ENOSHARE") == 0);
  #endif
  /* HP-UX, OSF/1 */
  #if defined ENOSYM
  ASSERT (strcmp (strerrorname_np (ENOSYM), "ENOSYM") == 0);
  #endif
  /* Solaris */
  #if defined ENOTACTIVE
  ASSERT (strcmp (strerrorname_np (ENOTACTIVE), "ENOTACTIVE") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Cygwin */
  #if defined ENOTBLK
  ASSERT (strcmp (strerrorname_np (ENOTBLK), "ENOTBLK") == 0);
  #endif
  /* FreeBSD */
  #if defined ENOTCAPABLE
  ASSERT (strcmp (strerrorname_np (ENOTCAPABLE), "ENOTCAPABLE") == 0);
  #endif
  /* IRIX */
  #if defined ENOTCONTROLLER
  ASSERT (strcmp (strerrorname_np (ENOTCONTROLLER), "ENOTCONTROLLER") == 0);
  #endif
  /* IRIX */
  #if defined ENOTENQUEUED
  ASSERT (strcmp (strerrorname_np (ENOTENQUEUED), "ENOTENQUEUED") == 0);
  #endif
  /* IRIX */
  #if defined ENOTJOINED
  ASSERT (strcmp (strerrorname_np (ENOTJOINED), "ENOTJOINED") == 0);
  #endif
  /* Linux, IRIX */
  #if defined ENOTNAM
  ASSERT (strcmp (strerrorname_np (ENOTNAM), "ENOTNAM") == 0);
  #endif
  /* AIX, Minix */
  #if defined ENOTREADY
  ASSERT (strcmp (strerrorname_np (ENOTREADY), "ENOTREADY") == 0);
  #endif
  /* AIX */
  #if defined ENOTRUST
  ASSERT (strcmp (strerrorname_np (ENOTRUST), "ENOTRUST") == 0);
  #endif
  /* IRIX */
  #if defined ENOTSTOPPED
  ASSERT (strcmp (strerrorname_np (ENOTSTOPPED), "ENOTSTOPPED") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ENOTUNIQ
  ASSERT (strcmp (strerrorname_np (ENOTUNIQ), "ENOTUNIQ") == 0);
  #endif
  /* HP-UX */
  #if defined ENOUNLD
  ASSERT (strcmp (strerrorname_np (ENOUNLD), "ENOUNLD") == 0);
  #endif
  /* HP-UX */
  #if defined ENOUNREG
  ASSERT (strcmp (strerrorname_np (ENOUNREG), "ENOUNREG") == 0);
  #endif
  /* Minix */
  #if defined ENOURG
  ASSERT (strcmp (strerrorname_np (ENOURG), "ENOURG") == 0);
  #endif
  /* native Windows */
  #if defined EOTHER
  ASSERT (strcmp (strerrorname_np (EOTHER), "EOTHER") == 0);
  #endif
  /* Minix */
  #if defined EPACKSIZE
  ASSERT (strcmp (strerrorname_np (EPACKSIZE), "EPACKSIZE") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Haiku, Cygwin */
  #if defined EPFNOSUPPORT
  ASSERT (strcmp (strerrorname_np (EPFNOSUPPORT), "EPFNOSUPPORT") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, IRIX, OSF/1, Minix, Cygwin */
  #if defined EPROCLIM
  ASSERT (strcmp (strerrorname_np (EPROCLIM), "EPROCLIM") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix */
  #if defined EPROCUNAVAIL
  ASSERT (strcmp (strerrorname_np (EPROCUNAVAIL), "EPROCUNAVAIL") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix */
  #if defined EPROGMISMATCH
  ASSERT (strcmp (strerrorname_np (EPROGMISMATCH), "EPROGMISMATCH") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix */
  #if defined EPROGUNAVAIL
  ASSERT (strcmp (strerrorname_np (EPROGUNAVAIL), "EPROGUNAVAIL") == 0);
  #endif
  /* Mac OS X */
  #if defined EPWROFF
  ASSERT (strcmp (strerrorname_np (EPWROFF), "EPWROFF") == 0);
  #endif
  /* Mac OS X */
  #if defined EQFULL
  ASSERT (strcmp (strerrorname_np (EQFULL), "EQFULL") == 0);
  #endif
  /* HP-UX */
  #if defined ERELOC
  ASSERT (strcmp (strerrorname_np (ERELOC), "ERELOC") == 0);
  #endif
  /* OSF/1 */
  #if defined ERELOCATED
  ASSERT (strcmp (strerrorname_np (ERELOCATED), "ERELOCATED") == 0);
  #endif
  /* FreeBSD */
  #if defined ERELOOKUP
  ASSERT (strcmp (strerrorname_np (ERELOOKUP), "ERELOOKUP") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EREMCHG
  ASSERT (strcmp (strerrorname_np (EREMCHG), "EREMCHG") == 0);
  #endif
  /* IRIX */
  #if defined EREMDEV
  ASSERT (strcmp (strerrorname_np (EREMDEV), "EREMDEV") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Haiku, Cygwin */
  #if defined EREMOTE
  ASSERT (strcmp (strerrorname_np (EREMOTE), "EREMOTE") == 0);
  #endif
  /* Linux, IRIX */
  #if defined EREMOTEIO
  ASSERT (strcmp (strerrorname_np (EREMOTEIO), "EREMOTEIO") == 0);
  #endif
  /* HP-UX */
  #if defined EREMOTERELEASE
  ASSERT (strcmp (strerrorname_np (EREMOTERELEASE), "EREMOTERELEASE") == 0);
  #endif
  /* Linux, FreeBSD, AIX, IRIX, OSF/1, Solaris, Minix */
  #if defined ERESTART
  ASSERT (strcmp (strerrorname_np (ERESTART), "ERESTART") == 0);
  #endif
  /* Linux */
  #if defined ERFKILL
  ASSERT (strcmp (strerrorname_np (ERFKILL), "ERFKILL") == 0);
  #endif
  /* Mac OS X, FreeBSD, NetBSD, OpenBSD, OSF/1, Minix */
  #if defined ERPCMISMATCH
  ASSERT (strcmp (strerrorname_np (ERPCMISMATCH), "ERPCMISMATCH") == 0);
  #endif
  /* AIX */
  #if defined ESAD
  ASSERT (strcmp (strerrorname_np (ESAD), "ESAD") == 0);
  #endif
  /* Mac OS X */
  #if defined ESHLIBVERS
  ASSERT (strcmp (strerrorname_np (ESHLIBVERS), "ESHLIBVERS") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Haiku, Cygwin */
  #if defined ESHUTDOWN
  ASSERT (strcmp (strerrorname_np (ESHUTDOWN), "ESHUTDOWN") == 0);
  #endif
  /* Haiku */
  #if defined ESIGPARM
  ASSERT (strcmp (strerrorname_np (ESIGPARM), "ESIGPARM") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Cygwin */
  #if defined ESOCKTNOSUPPORT
  ASSERT (strcmp (strerrorname_np (ESOCKTNOSUPPORT), "ESOCKTNOSUPPORT") == 0);
  #endif
  /* AIX, OSF/1 */
  #if defined ESOFT
  ASSERT (strcmp (strerrorname_np (ESOFT), "ESOFT") == 0);
  #endif
  /* Linux, HP-UX, IRIX, Solaris, Cygwin */
  #if defined ESRMNT
  ASSERT (strcmp (strerrorname_np (ESRMNT), "ESRMNT") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined ESTRPIPE
  ASSERT (strcmp (strerrorname_np (ESTRPIPE), "ESTRPIPE") == 0);
  #endif
  /* OSF/1 */
  #if defined ESUCCESS
  ASSERT (strcmp (strerrorname_np (ESUCCESS), "ESUCCESS") == 0);
  #endif
  /* AIX */
  #if defined ESYSERROR
  ASSERT (strcmp (strerrorname_np (ESYSERROR), "ESYSERROR") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Cygwin */
  #if defined ETOOMANYREFS
  ASSERT (strcmp (strerrorname_np (ETOOMANYREFS), "ETOOMANYREFS") == 0);
  #endif
  /* Minix */
  #if defined ETRAPDENIED
  ASSERT (strcmp (strerrorname_np (ETRAPDENIED), "ETRAPDENIED") == 0);
  #endif
  /* Linux, IRIX */
  #if defined EUCLEAN
  ASSERT (strcmp (strerrorname_np (EUCLEAN), "EUCLEAN") == 0);
  #endif
  /* Linux, AIX, HP-UX, IRIX, Solaris, Cygwin */
  #if defined EUNATCH
  ASSERT (strcmp (strerrorname_np (EUNATCH), "EUNATCH") == 0);
  #endif
  /* Minix */
  #if defined EURG
  ASSERT (strcmp (strerrorname_np (EURG), "EURG") == 0);
  #endif
  /* Linux, Mac OS X, FreeBSD, NetBSD, OpenBSD, AIX, HP-UX, IRIX, OSF/1, Solaris, Minix, Cygwin */
  #if defined EUSERS
  ASSERT (strcmp (strerrorname_np (EUSERS), "EUSERS") == 0);
  #endif
  /* OSF/1 */
  #if defined EVERSION
  ASSERT (strcmp (strerrorname_np (EVERSION), "EVERSION") == 0);
  #endif
  /* IRIX */
  #if defined EWRONGFS
  ASSERT (strcmp (strerrorname_np (EWRONGFS), "EWRONGFS") == 0);
  #endif
  /* AIX */
  #if defined EWRPROTECT
  ASSERT (strcmp (strerrorname_np (EWRPROTECT), "EWRPROTECT") == 0);
  #endif
  /* Linux, IRIX, Solaris, Cygwin */
  #if defined EXFULL
  ASSERT (strcmp (strerrorname_np (EXFULL), "EXFULL") == 0);
  #endif

  ASSERT (strerrorname_np (20200830) == NULL);
  ASSERT (strerrorname_np (-714) == NULL);

  return 0;
}
