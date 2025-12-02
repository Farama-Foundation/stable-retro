/*  Cheat Support for PCSX-Reloaded
 *  Copyright (C) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#ifndef CHEAT_H
#define CHEAT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t	Addr;
	uint16_t	Val;
	uint16_t	OldVal;
} CheatCode;

typedef struct {
	char		*Descr;
	int			First;		// index of the first cheat code
	int			n;			// number of cheat codes for this cheat
	int			Enabled;
	int			WasEnabled;
} Cheat;

void ClearAllCheats();

void LoadCheats(const char *filename);
void SaveCheats(const char *filename);

void ApplyCheats();

int AddCheat(const char *descr, char *code);
void RemoveCheat(int index);
int EditCheat(int index, const char *descr, char *code);

void FreeCheatSearchResults();
void FreeCheatSearchMem();
void CheatSearchBackupMemory();

void CheatSearchEqual8(u8 val);
void CheatSearchEqual16(u16 val);
void CheatSearchEqual32(u32 val);
void CheatSearchNotEqual8(u8 val);
void CheatSearchNotEqual16(u16 val);
void CheatSearchNotEqual32(u32 val);
void CheatSearchRange8(u8 min, u8 max);
void CheatSearchRange16(u16 min, u16 max);
void CheatSearchRange32(u32 min, u32 max);
void CheatSearchIncreasedBy8(u8 val);
void CheatSearchIncreasedBy16(u16 val);
void CheatSearchIncreasedBy32(u32 val);
void CheatSearchDecreasedBy8(u8 val);
void CheatSearchDecreasedBy16(u16 val);
void CheatSearchDecreasedBy32(u32 val);
void CheatSearchIncreased8();
void CheatSearchIncreased16();
void CheatSearchIncreased32();
void CheatSearchDecreased8();
void CheatSearchDecreased16();
void CheatSearchDecreased32();
void CheatSearchDifferent8();
void CheatSearchDifferent16();
void CheatSearchDifferent32();
void CheatSearchNoChange8();
void CheatSearchNoChange16();
void CheatSearchNoChange32();

extern Cheat *Cheats;
extern CheatCode *CheatCodes;
extern int NumCheats;
extern int NumCodes;

extern s8 *prevM;
extern u32 *SearchResults;
extern int NumSearchResults;

extern int NumCheatsAllocated;
extern int NumCodesAllocated;

#define PREVM(mem)		(&prevM[mem])
#define PrevMu8(mem)	(*(u8 *)PREVM(mem))
#define PrevMu16(mem)	(SWAP16(*(u16 *)PREVM(mem)))
#define PrevMu32(mem)	(SWAP32(*(u32 *)PREVM(mem)))

// cheat types
#define CHEAT_CONST8        0x30	/* 8-bit Constant Write */
#define CHEAT_CONST16       0x80	/* 16-bit Constant Write */

#define CHEAT_INC16         0x10	/* 16-bit Increment */
#define CHEAT_DEC16         0x11	/* 16-bit Decrement */
#define CHEAT_SCRATCHPAD16  0x1F	/* 16-bit Scratchpad Write */
#define CHEAT_INC8          0x20	/* 8-bit Increment */
#define CHEAT_DEC8          0x21	/* 8-bit Decrement */
#define CHEAT_SLIDE         0x50	/* Slide Codes */
#define CHEAT_MEMCPY        0xC2	/* Memory Copy */

// conditionals
#define CHEAT_EQU16         0xD0	/* 16-bit Equal To */
#define CHEAT_NOTEQU16      0xD1	/* 16-bit Not Equal To */
#define CHEAT_LESSTHAN16    0xD2	/* 16-bit Less Than */
#define CHEAT_GREATERTHAN16 0xD3	/* 16-bit Greater Than */
#define CHEAT_BUTTONS1_16   0xD4	/* button presses equate to YYYY */
#define CHEAT_EQU8          0xE0	/* 8-bit Equal To */
#define CHEAT_NOTEQU8       0xE1	/* 8-bit Not Equal To */
#define CHEAT_LESSTHAN8     0xE2	/* 8-bit Less Than */
#define CHEAT_GREATERTHAN8  0xE3	/* 8-bit Greater Than */

#ifdef __cplusplus
}
#endif
#endif
