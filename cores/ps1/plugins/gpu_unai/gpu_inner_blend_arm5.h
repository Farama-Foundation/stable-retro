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
	asm ("and  %[src], %[src], %[msk]  " : [src] "=r" (uSrc) : "0" (uSrc), [msk] "r" (uMsk)                  ); \
	asm ("and  %[dst], %[dst], %[msk]  " : [dst] "=r" (uDst) : "0" (uDst), [msk] "r" (uMsk)                  ); \
	asm ("add  %[src], %[dst], %[src]  " : [src] "=r" (uSrc) :             [dst] "r" (uDst), "0" (uSrc)      ); \
	asm ("mov  %[src], %[src], lsr #1  " : [src] "=r" (uSrc) : "0" (uSrc)                                    ); \
}

//	1.0 x Back + 1.0 x Forward
#define gpuBlending01(uSrc,uDst) \
{ \
	u16 st,dt,out; \
	asm ("and    %[dt],  %[dst],   #0x7C00  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x7C00  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("add    %[out], %[dt],    %[st]    " : [out] "=r" (out)  :             [dt]  "r" (dt),   [st]  "r" (st)    ); \
	asm ("cmp    %[out], #0x7C00            " :                   :             [out] "r" (out) : "cc"              ); \
	asm ("movhi  %[out], #0x7C00            " : [out] "=r" (out)  : "0" (out)                                       ); \
	asm ("and    %[dt],  %[dst],   #0x03E0  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x03E0  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("add    %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st)                      ); \
	asm ("cmp    %[dt],  #0x03E0            " :                   :             [dt]  "r" (dt) : "cc"               ); \
	asm ("movhi  %[dt],  #0x03E0            " : [dt]  "=r" (dt)   : "0" (dt)                                        ); \
	asm ("orr    %[out], %[out],   %[dt]    " : [out] "=r" (out)  : "0" (out),  [dt]  "r" (dt)                      ); \
	asm ("and    %[dt],  %[dst],   #0x001F  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x001F  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("add    %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st)                      ); \
	asm ("cmp    %[dt],  #0x001F            " :                   :             [dt]  "r" (dt) : "cc"               ); \
	asm ("movhi  %[dt],  #0x001F            " : [dt]  "=r" (dt)   : "0" (dt)                                        ); \
	asm ("orr    %[uSrc], %[out],   %[dt]   " : [uSrc] "=r" (uSrc)  : [out] "r" (out),  [dt]  "r" (dt)              ); \
}

//	1.0 x Back - 1.0 x Forward	*/
#define gpuBlending02(uSrc,uDst) \
{ \
	u16 st,dt,out; \
	asm ("and    %[dt],  %[dst],   #0x7C00  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x7C00  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("subs   %[out], %[dt],    %[st]    " : [out] "=r" (out)  : [dt]  "r" (dt),   [st]  "r" (st) : "cc"         ); \
	asm ("movmi  %[out], #0x0000            " : [out] "=r" (out)  : "0" (out)                                       ); \
	asm ("and    %[dt],  %[dst],   #0x03E0  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x03E0  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("subs   %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st) : "cc"               ); \
	asm ("orrpl  %[out], %[out],   %[dt]    " : [out] "=r" (out)  : "0" (out),  [dt]  "r" (dt)                      ); \
	asm ("and    %[dt],  %[dst],   #0x001F  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
	asm ("and    %[st],  %[src],   #0x001F  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
	asm ("subs   %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st) : "cc"               ); \
	asm ("orrpl  %[out], %[out],   %[dt]    " : [out] "=r" (out)  : "0" (out),  [dt]  "r" (dt)                      ); \
	asm ("mov %[uSrc], %[out]" : [uSrc] "=r" (uSrc) : [out] "r" (out) ); \
}

//	1.0 x Back + 0.25 x Forward	*/
#define gpuBlending03(uSrc,uDst) \
{ \
		u16 st,dt,out; \
		asm ("mov    %[src], %[src],   lsr #2   " : [src] "=r" (uSrc) : "0" (uSrc)                                      ); \
		asm ("and    %[dt],  %[dst],   #0x7C00  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
		asm ("and    %[st],  %[src],   #0x1C00  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
		asm ("add    %[out], %[dt],    %[st]    " : [out] "=r" (out)  :             [dt]  "r" (dt),   [st]  "r" (st)    ); \
		asm ("cmp    %[out], #0x7C00            " :                   :             [out] "r" (out) : "cc"              ); \
		asm ("movhi  %[out], #0x7C00            " : [out] "=r" (out)  : "0" (out)                                       ); \
		asm ("and    %[dt],  %[dst],   #0x03E0  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
		asm ("and    %[st],  %[src],   #0x00E0  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
		asm ("add    %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st)                      ); \
		asm ("cmp    %[dt],  #0x03E0            " :                   :             [dt]  "r" (dt) : "cc"               ); \
		asm ("movhi  %[dt],  #0x03E0            " : [dt]  "=r" (dt)   : "0" (dt)                                        ); \
		asm ("orr    %[out], %[out],   %[dt]    " : [out] "=r" (out)  : "0" (out),  [dt]  "r" (dt)                      ); \
		asm ("and    %[dt],  %[dst],   #0x001F  " : [dt]  "=r" (dt)   :             [dst] "r" (uDst)                    ); \
		asm ("and    %[st],  %[src],   #0x0007  " : [st]  "=r" (st)   :             [src] "r" (uSrc)                    ); \
		asm ("add    %[dt],  %[dt],    %[st]    " : [dt]  "=r" (dt)   : "0" (dt),   [st]  "r" (st)                      ); \
		asm ("cmp    %[dt],  #0x001F            " :                   :             [dt]  "r" (dt) : "cc"               ); \
		asm ("movhi  %[dt],  #0x001F            " : [dt]  "=r" (dt)   : "0" (dt)                                        ); \
		asm ("orr    %[uSrc], %[out],   %[dt]   " : [uSrc] "=r" (uSrc)  : [out] "r" (out),  [dt]  "r" (dt)              ); \
}

#endif  //_OP_BLEND_H_
