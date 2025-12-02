/* Test of <stdio.h> substitute in C++ mode.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#include <stdio.h>

#include "signature.h"


#if GNULIB_TEST_DPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::dprintf, int, (int, const char *, ...));
#endif

#if GNULIB_TEST_FCLOSE
SIGNATURE_CHECK (GNULIB_NAMESPACE::fclose, int, (FILE *));
#endif

#if GNULIB_TEST_FDOPEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::fdopen, FILE *, (int, const char *));
#endif

#if GNULIB_TEST_FFLUSH
SIGNATURE_CHECK (GNULIB_NAMESPACE::fflush, int, (FILE *));
#endif

#if GNULIB_TEST_FGETC
SIGNATURE_CHECK (GNULIB_NAMESPACE::fgetc, int, (FILE *));
#endif

#if GNULIB_TEST_FGETS
SIGNATURE_CHECK (GNULIB_NAMESPACE::fgets, char *, (char *, int, FILE *));
#endif

#if GNULIB_TEST_FOPEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::fopen, FILE *,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_FPRINTF_POSIX || GNULIB_TEST_FPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::fprintf, int, (FILE *, const char *, ...));
#endif

#if GNULIB_TEST_FPURGE
SIGNATURE_CHECK (GNULIB_NAMESPACE::fpurge, int, (FILE *));
#endif

#if GNULIB_TEST_FPUTC
SIGNATURE_CHECK (GNULIB_NAMESPACE::fputc, int, (int, FILE *));
#endif

#if GNULIB_TEST_FPUTS
SIGNATURE_CHECK (GNULIB_NAMESPACE::fputs, int, (const char *, FILE *));
#endif

#if GNULIB_TEST_FREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::fread, size_t,
                 (void *, size_t, size_t, FILE *));
#endif

#if GNULIB_TEST_FREOPEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::freopen, FILE *,
                 (const char *, const char *, FILE *));
#endif

#if GNULIB_TEST_FSCANF
SIGNATURE_CHECK (GNULIB_NAMESPACE::fscanf, int, (FILE *, const char *, ...));
#endif

#if GNULIB_TEST_FSEEK
SIGNATURE_CHECK (GNULIB_NAMESPACE::fseek, int, (FILE *, long, int));
#endif

#if GNULIB_TEST_FSEEKO
SIGNATURE_CHECK (GNULIB_NAMESPACE::fseeko, int, (FILE *, off_t, int));
#endif

#if GNULIB_TEST_FTELL
SIGNATURE_CHECK (GNULIB_NAMESPACE::ftell, long, (FILE *));
#endif

#if GNULIB_TEST_FTELLO
SIGNATURE_CHECK (GNULIB_NAMESPACE::ftello, off_t, (FILE *));
#endif

#if GNULIB_TEST_FWRITE
SIGNATURE_CHECK (GNULIB_NAMESPACE::fwrite, size_t,
                 (const void *, size_t, size_t, FILE *));
#endif

#if GNULIB_TEST_GETC
SIGNATURE_CHECK (GNULIB_NAMESPACE::getc, int, (FILE *));
#endif

#if GNULIB_TEST_GETCHAR
SIGNATURE_CHECK (GNULIB_NAMESPACE::getchar, int, (void));
#endif

#if GNULIB_TEST_GETDELIM
SIGNATURE_CHECK (GNULIB_NAMESPACE::getdelim, ssize_t,
                 (char **, size_t *, int, FILE *));
#endif

#if GNULIB_TEST_GETLINE
SIGNATURE_CHECK (GNULIB_NAMESPACE::getline, ssize_t,
                 (char **, size_t *, FILE *));
#endif

/* Don't bother testing gets; it should never be used.  */

#if GNULIB_TEST_OBSTACK_PRINTF || GNULIB_TEST_OBSTACK_PRINTF_POSIX
SIGNATURE_CHECK (GNULIB_NAMESPACE::obstack_printf, int,
                 (struct obstack *, const char *, ...));
SIGNATURE_CHECK (GNULIB_NAMESPACE::obstack_vprintf, int,
                 (struct obstack *, const char *, va_list));
#endif

#if GNULIB_TEST_PCLOSE
SIGNATURE_CHECK (GNULIB_NAMESPACE::pclose, int, (FILE *));
#endif

//SIGNATURE_CHECK (GNULIB_NAMESPACE::pclose, int, (FILE *));

#if GNULIB_TEST_PERROR
SIGNATURE_CHECK (GNULIB_NAMESPACE::perror, void, (const char *));
#endif

#if GNULIB_TEST_POPEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::popen, FILE *,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_PRINTF_POSIX || GNULIB_TEST_PRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::printf, int, (const char *, ...));
#endif

#if GNULIB_TEST_PUTC
SIGNATURE_CHECK (GNULIB_NAMESPACE::putc, int, (int, FILE *));
#endif

#if GNULIB_TEST_PUTCHAR
SIGNATURE_CHECK (GNULIB_NAMESPACE::putchar, int, (int));
#endif

#if GNULIB_TEST_PUTS
SIGNATURE_CHECK (GNULIB_NAMESPACE::puts, int, (const char *));
#endif

#if GNULIB_TEST_REMOVE
SIGNATURE_CHECK (GNULIB_NAMESPACE::remove, int, (const char *));
#endif

#if GNULIB_TEST_RENAME
SIGNATURE_CHECK (GNULIB_NAMESPACE::rename, int, (const char *, const char *));
#endif

#if GNULIB_TEST_RENAMEAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::renameat, int,
                 (int, char const *, int, char const *));
#endif

#if GNULIB_TEST_SCANF
SIGNATURE_CHECK (GNULIB_NAMESPACE::scanf, int, (const char *, ...));
#endif

#if GNULIB_TEST_SNPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::snprintf, int,
                 (char *, size_t, const char *, ...));
#endif

#if GNULIB_TEST_SPRINTF_POSIX
SIGNATURE_CHECK (GNULIB_NAMESPACE::sprintf, int, (char *, const char *, ...));
#endif

#if GNULIB_TEST_TMPFILE
SIGNATURE_CHECK (GNULIB_NAMESPACE::tmpfile, FILE *, (void));
#endif

#if GNULIB_TEST_VASPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::asprintf, int,
                 (char **, const char *, ...));
SIGNATURE_CHECK (GNULIB_NAMESPACE::vasprintf, int,
                 (char **, const char *, va_list));
#endif

#if GNULIB_TEST_VDPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vdprintf, int,
                 (int, const char *, va_list));
#endif

#if GNULIB_TEST_VFPRINTF_POSIX || GNULIB_TEST_VFPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vfprintf, int,
                 (FILE *, const char *, va_list));
#endif

#if GNULIB_TEST_VFSCANF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vfscanf, int,
                 (FILE *, const char *, va_list));
#endif

#if GNULIB_TEST_VPRINTF_POSIX || GNULIB_TEST_VPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vprintf, int, (const char *, va_list));
#endif

#if GNULIB_TEST_VSCANF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vscanf, int, (const char *, va_list));
#endif

#if GNULIB_TEST_VSNPRINTF
SIGNATURE_CHECK (GNULIB_NAMESPACE::vsnprintf, int,
                 (char *, size_t, const char *, va_list));
#endif

#if GNULIB_TEST_VSPRINTF_POSIX
SIGNATURE_CHECK (GNULIB_NAMESPACE::vsprintf, int,
                 (char *, const char *, va_list));
#endif


int
main ()
{
}
