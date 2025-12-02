/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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

#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define SWAP16(v) __builtin_bswap16(v)
#define SWAP32(v) __builtin_bswap32(v)
#define SWAPu32(v) SWAP32((u32)(v))
#define SWAPs32(v) SWAP32((s32)(v))

#define SWAPu16(v) SWAP16((u16)(v))
#define SWAPs16(v) SWAP16((s16)(v))

#else

#define SWAP16(b) (b)
#define SWAP32(b) (b)

#define SWAPu16(b) (b)
#define SWAPu32(b) (b)

#endif

#ifdef LIGHTREC
#define INVALID_PTR ((void *)-1)
#else
#define INVALID_PTR NULL
#endif

extern s8 *psxM;
#define psxMs8(mem)		psxM[(mem) & 0x1fffff]
#define psxMs16(mem)	(SWAP16(*(s16 *)&psxM[(mem) & 0x1fffff]))
#define psxMs32(mem)	(SWAP32(*(s32 *)&psxM[(mem) & 0x1fffff]))
#define psxMu8(mem)		(*(u8 *)&psxM[(mem) & 0x1fffff])
#define psxMu16(mem)	(SWAP16(*(u16 *)&psxM[(mem) & 0x1fffff]))
#define psxMu32(mem)	(SWAP32(*(u32 *)&psxM[(mem) & 0x1fffff]))

#define psxMs8ref(mem)	psxM[(mem) & 0x1fffff]
#define psxMs16ref(mem)	(*(s16 *)&psxM[(mem) & 0x1fffff])
#define psxMs32ref(mem)	(*(s32 *)&psxM[(mem) & 0x1fffff])
#define psxMu8ref(mem)	(*(u8 *)&psxM[(mem) & 0x1fffff])
#define psxMu16ref(mem)	(*(u16 *)&psxM[(mem) & 0x1fffff])
#define psxMu32ref(mem)	(*(u32 *)&psxM[(mem) & 0x1fffff])

extern s8 *psxP;
#define psxPs8(mem)	    psxP[(mem) & 0xffff]
#define psxPs16(mem)	(SWAP16(*(s16 *)&psxP[(mem) & 0xffff]))
#define psxPs32(mem)	(SWAP32(*(s32 *)&psxP[(mem) & 0xffff]))
#define psxPu8(mem)		(*(u8 *)&psxP[(mem) & 0xffff])
#define psxPu16(mem)	(SWAP16(*(u16 *)&psxP[(mem) & 0xffff]))
#define psxPu32(mem)	(SWAP32(*(u32 *)&psxP[(mem) & 0xffff]))

#define psxPs8ref(mem)	psxP[(mem) & 0xffff]
#define psxPs16ref(mem)	(*(s16 *)&psxP[(mem) & 0xffff])
#define psxPs32ref(mem)	(*(s32 *)&psxP[(mem) & 0xffff])
#define psxPu8ref(mem)	(*(u8 *)&psxP[(mem) & 0xffff])
#define psxPu16ref(mem)	(*(u16 *)&psxP[(mem) & 0xffff])
#define psxPu32ref(mem)	(*(u32 *)&psxP[(mem) & 0xffff])

extern s8 *psxR;
#define psxRs8(mem)		psxR[(mem) & 0x7ffff]
#define psxRs16(mem)	(SWAP16(*(s16 *)&psxR[(mem) & 0x7ffff]))
#define psxRs32(mem)	(SWAP32(*(s32 *)&psxR[(mem) & 0x7ffff]))
#define psxRu8(mem)		(*(u8* )&psxR[(mem) & 0x7ffff])
#define psxRu16(mem)	(SWAP16(*(u16 *)&psxR[(mem) & 0x7ffff]))
#define psxRu32(mem)	(SWAP32(*(u32 *)&psxR[(mem) & 0x7ffff]))

#define psxRs8ref(mem)	psxR[(mem) & 0x7ffff]
#define psxRs16ref(mem)	(*(s16*)&psxR[(mem) & 0x7ffff])
#define psxRs32ref(mem)	(*(s32*)&psxR[(mem) & 0x7ffff])
#define psxRu8ref(mem)	(*(u8 *)&psxR[(mem) & 0x7ffff])
#define psxRu16ref(mem)	(*(u16*)&psxR[(mem) & 0x7ffff])
#define psxRu32ref(mem)	(*(u32*)&psxR[(mem) & 0x7ffff])

extern s8 *psxH;
#define psxHs8(mem)		psxH[(mem) & 0xffff]
#define psxHs16(mem)	(SWAP16(*(s16 *)&psxH[(mem) & 0xffff]))
#define psxHs32(mem)	(SWAP32(*(s32 *)&psxH[(mem) & 0xffff]))
#define psxHu8(mem)		(*(u8 *)&psxH[(mem) & 0xffff])
#define psxHu16(mem)	(SWAP16(*(u16 *)&psxH[(mem) & 0xffff]))
#define psxHu32(mem)	(SWAP32(*(u32 *)&psxH[(mem) & 0xffff]))

#define psxHs8ref(mem)	psxH[(mem) & 0xffff]
#define psxHs16ref(mem)	(*(s16 *)&psxH[(mem) & 0xffff])
#define psxHs32ref(mem)	(*(s32 *)&psxH[(mem) & 0xffff])
#define psxHu8ref(mem)	(*(u8 *)&psxH[(mem) & 0xffff])
#define psxHu16ref(mem)	(*(u16 *)&psxH[(mem) & 0xffff])
#define psxHu32ref(mem)	(*(u32 *)&psxH[(mem) & 0xffff])

extern u8 **psxMemWLUT;
extern u8 **psxMemRLUT;
extern int cache_isolated;

#ifndef DISABLE_MEM_LUTS
#define DISABLE_MEM_LUTS 0
#endif

static inline void * psxm_lut(u32 mem, int write, u8 **lut)
{
	if (!DISABLE_MEM_LUTS) {
		void *ptr = lut[mem >> 16];

		return ptr == INVALID_PTR ? INVALID_PTR
			: (void *)((uintptr_t)ptr + (u16)mem);
	}

	if (mem >= 0xa0000000)
		mem -= 0xa0000000;
	else
		mem &= ~0x80000000;

	if (mem < 0x800000) {
		if (cache_isolated)
			return INVALID_PTR;

		return &psxM[mem & 0x1fffff];
	}

	if (mem > 0x1f800000 && mem <= 0x1f810000)
		return &psxH[mem - 0x1f800000];

	if (!write) {
		if (mem > 0x1fc00000 && mem <= 0x1fc80000)
			return &psxR[mem - 0x1fc00000];

		if (mem > 0x1f000000 && mem <= 0x1f010000)
			return &psxP[mem - 0x1f000000];
	}

	return INVALID_PTR;
}

static inline void * psxm(u32 mem, int write)
{
	return psxm_lut(mem, write, write ? psxMemWLUT : psxMemRLUT);
}

#define PSXM(mem) psxm(mem, 0)
#define PSXMs8(mem)		(*(s8 *)PSXM(mem))
#define PSXMs16(mem)	(SWAP16(*(s16 *)PSXM(mem)))
#define PSXMs32(mem)	(SWAP32(*(s32 *)PSXM(mem)))
#define PSXMu8(mem)		(*(u8 *)PSXM(mem))
#define PSXMu16(mem)	(SWAP16(*(u16 *)PSXM(mem)))
#define PSXMu32(mem)	(SWAP32(*(u32 *)PSXM(mem)))

#define PSXMu32ref(mem)	(*(u32 *)PSXM(mem))

int psxMemInit();
void psxMemReset();
void psxMemOnIsolate(int enable);
void psxMemShutdown();

u8 psxMemRead8 (u32 mem);
u16 psxMemRead16(u32 mem);
u32 psxMemRead32(u32 mem);
void psxMemWrite8 (u32 mem, u32 value);
void psxMemWrite16(u32 mem, u32 value);
void psxMemWrite32(u32 mem, u32 value);
void *psxMemPointer(u32 mem);

#ifdef __cplusplus
}
#endif
#endif
