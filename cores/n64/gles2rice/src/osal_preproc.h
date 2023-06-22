/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - osal_preproc.h                                          *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* this header file is for system-dependent #defines, #includes, and typedefs */

#ifndef OSAL_PREPROC_H
#define OSAL_PREPROC_H

#include <boolean.h>

#ifdef _MSC_VER
#define ALIGN(BYTES,DATA) __declspec(align(BYTES)) DATA
#else
#define ALIGN(BYTES,DATA) DATA __attribute__((aligned(BYTES)))
#endif

typedef struct
{
   int top;
   int bottom;
   int right;
   int left;
} M64P_RECT;

#endif // OSAL_PREPROC_H
