/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#ifndef _OP_BLEND_H_
#define _OP_BLEND_H_

//  GPU Blending operations functions

#define gpuBlending00(uSrc,uDst) \
{ \
	asm ("and  %[src], %[src], %[msk]\n" \
	     "and  %[dst], %[dst], %[msk]\n" \
	     "add  %[src], %[dst], %[src]\n" \
	     "mov  %[src], %[src], lsr #1\n" \
	 : [src] "=&r" (uSrc), [dst] "=&r" (uDst) : "0" (uSrc), "1" (uDst), [msk] "r" (uMsk)); \
}

//	1.0 x Back + 1.0 x Forward
#define gpuBlending01(uSrc,uDst) \
{ \
	u32 st,dt,out; \
	asm ("and    %[dt],  %[dst],   #0x7C00\n" \
	     "and    %[st],  %[src],   #0x7C00\n" \
	     "add    %[out], %[dt],    %[st]  \n" \
	     "cmp    %[out], #0x7C00          \n" \
	     "movhi  %[out], #0x7C00          \n" \
	     "and    %[dt],  %[dst],   #0x03E0\n" \
	     "and    %[st],  %[src],   #0x03E0\n" \
	     "add    %[dt],  %[dt],    %[st]  \n" \
	     "cmp    %[dt],  #0x03E0          \n" \
	     "movhi  %[dt],  #0x03E0          \n" \
	     "orr    %[out], %[out],   %[dt]  \n" \
	     "and    %[dt],  %[dst],   #0x001F\n" \
	     "and    %[st],  %[src],   #0x001F\n" \
	     "add    %[dt],  %[dt],    %[st]  \n" \
	     "cmp    %[dt],  #0x001F          \n" \
	     "movhi  %[dt],  #0x001F          \n" \
	     "orr    %[src], %[out],  %[dt]  \n" \
	 : [src] "=r" (uSrc), [st] "=&r" (st), [dt] "=&r" (dt), [out] "=&r" (out) \
	 : [dst] "r" (uDst), "0" (uSrc) : "cc"); \
}

//	1.0 x Back - 1.0 x Forward	*/
#define gpuBlending02(uSrc,uDst) \
{ \
	u32 st,dt,out; \
	asm ("and    %[dt],  %[dst],   #0x7C00\n" \
	     "and    %[st],  %[src],   #0x7C00\n" \
	     "subs   %[out], %[dt],    %[st]  \n" \
	     "movmi  %[out], #0x0000          \n" \
	     "and    %[dt],  %[dst],   #0x03E0\n" \
	     "and    %[st],  %[src],   #0x03E0\n" \
	     "subs   %[dt],  %[dt],    %[st]  \n" \
	     "orrpl  %[out], %[out],   %[dt]  \n" \
	     "and    %[dt],  %[dst],   #0x001F\n" \
	     "and    %[st],  %[src],   #0x001F\n" \
	     "subs   %[dt],  %[dt],    %[st]  \n" \
	     "orrpl  %[out], %[out],   %[dt]  \n" \
	     "mov    %[src], %[out]           \n" \
	 : [src] "=r" (uSrc), [st] "=&r" (st), [dt] "=&r" (dt), [out] "=&r" (out) \
	 : [dst] "r" (uDst), "0" (uSrc) : "cc"); \
}

//	1.0 x Back + 0.25 x Forward	*/
#define gpuBlending03(uSrc,uDst) \
{ \
	u32 st,dt,out; \
	asm ("mov    %[src], %[src],   lsr #2 \n" \
	     "and    %[dt],  %[dst],   #0x7C00\n" \
	     "and    %[st],  %[src],   #0x1C00\n" \
	     "add    %[out], %[dt],    %[st]  \n" \
	     "cmp    %[out], #0x7C00          \n" \
	     "movhi  %[out], #0x7C00          \n" \
	     "and    %[dt],  %[dst],   #0x03E0\n" \
	     "and    %[st],  %[src],   #0x00E0\n" \
	     "add    %[dt],  %[dt],    %[st]  \n" \
	     "cmp    %[dt],  #0x03E0          \n" \
	     "movhi  %[dt],  #0x03E0          \n" \
	     "orr    %[out], %[out],   %[dt]  \n" \
	     "and    %[dt],  %[dst],   #0x001F\n" \
	     "and    %[st],  %[src],   #0x0007\n" \
	     "add    %[dt],  %[dt],    %[st]  \n" \
	     "cmp    %[dt],  #0x001F          \n" \
	     "movhi  %[dt],  #0x001F          \n" \
	     "orr    %[src], %[out],   %[dt]  \n" \
	 : [src] "=r" (uSrc), [st] "=&r" (st), [dt] "=&r" (dt), [out] "=&r" (out) \
	 : [dst] "r" (uDst), "0" (uSrc) : "cc"); \
}

#endif  //_OP_BLEND_H_
