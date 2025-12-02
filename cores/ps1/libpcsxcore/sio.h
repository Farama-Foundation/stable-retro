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


#ifndef _SIO_H_
#define _SIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "plugins.h"
#include "psemu_plugin_defs.h"

#define MCD_SIZE	(1024 * 8 * 16)

extern char Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];
extern char McdDisable[2];

void sioWrite8(unsigned char value);
void sioWriteStat16(unsigned short value);
void sioWriteMode16(unsigned short value);
void sioWriteCtrl16(unsigned short value);
void sioWriteBaud16(unsigned short value);

unsigned char sioRead8();
unsigned short sioReadStat16();
unsigned short sioReadMode16();
unsigned short sioReadCtrl16();
unsigned short sioReadBaud16();

void sioInterrupt();
int sioFreeze(void *f, int Mode);

void LoadMcd(int mcd, char *str);
void LoadMcds(char *mcd1, char *mcd2);
void SaveMcd(char *mcd, char *data, uint32_t adr, int size);
void CreateMcd(char *mcd);
void ConvertMcd(char *mcd, char *data);

typedef struct {
	char Title[48 + 1]; // Title in ASCII
	char sTitle[48 * 2 + 1]; // Title in Shift-JIS
	char ID[12 + 1];
	char Name[16 + 1];
	int IconCount;
	short Icon[16 * 16 * 3];
	unsigned char Flags;
} McdBlock;

void GetMcdBlockInfo(int mcd, int block, McdBlock *info);

#ifdef __cplusplus
}
#endif
#endif
