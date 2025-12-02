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

#ifndef __CDROM_H__
#define __CDROM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "decode_xa.h"
#include "r3000a.h"
#include "plugins.h"
#include "psxmem.h"
#include "psxhw.h"

#define btoi(b)     ((b) / 16 * 10 + (b) % 16) /* BCD to u_char */
#define itob(i)     ((i) / 10 * 16 + (i) % 10) /* u_char to BCD */

#define ABS_CD(x) ((x >= 0) ? x : -x)
#define MIN_VALUE(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define MAX_VALUE(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

#define CD_FRAMESIZE_RAW		2352
#define DATA_SIZE				(CD_FRAMESIZE_RAW - 12)

/* CD_FRAMESIZE_RAW aligned to a cache line for DMA buffers
 * (assuming a cache line of max. 64 bytes) */
#define CD_FRAMESIZE_RAW_ALIGNED	2368

#define SUB_FRAMESIZE			96

#define MSF2SECT(m, s, f)		(((m) * 60 + (s) - 2) * 75 + (f))

static inline void lba2msf(unsigned int lba, u8 *m, u8 *s, u8 *f) {
	*m = lba / 75 / 60;
	lba = lba - *m * 75 * 60;
	*s = lba / 75;
	lba = lba - *s * 75;
	*f = lba;
}

void cdrReset();

void cdrInterrupt(void);
void cdrPlayReadInterrupt(void);
void cdrLidSeekInterrupt(void);
void cdrDmaInterrupt(void);
void LidInterrupt(void);
unsigned char cdrRead0(void);
unsigned char cdrRead1(void);
unsigned char cdrRead2(void);
unsigned char cdrRead3(void);
void cdrWrite0(unsigned char rt);
void cdrWrite1(unsigned char rt);
void cdrWrite2(unsigned char rt);
void cdrWrite3(unsigned char rt);
int cdrFreeze(void *f, int Mode);

#ifdef __cplusplus
}
#endif
#endif
