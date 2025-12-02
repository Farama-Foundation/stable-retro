/***************************************************************************
                           StdAfx.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef __P_STDAFX_H__
#define __P_STDAFX_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#undef CALLBACK
#define CALLBACK
#define DWORD unsigned int
#define LOWORD(l)           ((unsigned short)(l))
#define HIWORD(l)           ((unsigned short)(((unsigned int)(l) >> 16) & 0xFFFF))

#ifndef INLINE
#define INLINE static inline
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HTOLE16(x) __builtin_bswap16(x)
#define LE16TOH(x) __builtin_bswap16(x)
#else
#define HTOLE16(x) (x)
#define LE16TOH(x) (x)
#endif

#include "psemuxa.h"

#endif /* __P_STDAFX_H__ */
