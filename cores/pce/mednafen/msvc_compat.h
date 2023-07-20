/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __RARCH_MSVC_COMPAT_H
#define __RARCH_MSVC_COMPAT_H

#ifdef _MSC_VER

#undef UNICODE // Do not bother with UNICODE at this time.

#include <stddef.h>
#include <io.h>
#include <math.h>

// Python headers defines ssize_t and sets HAVE_SSIZE_T. Cannot duplicate these efforts.
#ifndef HAVE_SSIZE_T
#if defined(_WIN64)
typedef __int64 ssize_t;
#elif defined(_WIN32)
typedef int ssize_t;
#endif
#endif

#define snprintf _snprintf
#define strtoll _strtoi64
#define strtoull _strtoui64
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup
#define lseek _lseek

#if (_MSC_VER < 1310)
#include <tchar.h>
#define strlen _tcslen
#endif

# define S_IRUSR  S_IREAD                      /* read, user */
# define S_IWUSR  S_IWRITE                     /* write, user */

// Disable some of the annoying warnings.
#pragma warning(disable : 4800)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)
#pragma warning(disable : 4146)
#pragma warning(disable : 4267)

#if _MSC_VER < 1800
#define roundf(in) (in >= 0.0f ? floorf(in + 0.5f) : ceilf(in - 0.5f))
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#endif
#endif
