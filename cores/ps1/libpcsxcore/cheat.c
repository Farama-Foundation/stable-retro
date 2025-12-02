/*  Cheat Support for PCSX-Reloaded
 *  Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "misc.h"
#include "../frontend/plugin_lib.h" // in_keystate for D4

#include "cheat.h"

Cheat *Cheats = NULL;
int NumCheats = 0;
int NumCheatsAllocated = 0;

CheatCode *CheatCodes = NULL;
int NumCodes = 0;
int NumCodesAllocated = 0;

s8 *prevM = NULL;
u32 *SearchResults = NULL;
int NumSearchResults = 0;
static int NumSearchResultsAllocated = 0;

#define ALLOC_INCREMENT		100

void ClearAllCheats() {
	int i;

	if (Cheats != NULL) {
		for (i = 0; i < NumCheats; i++) {
			free(Cheats[i].Descr);
		}
		free(Cheats);
	}

	Cheats = NULL;
	NumCheats = 0;
	NumCheatsAllocated = 0;

	if (CheatCodes != NULL) {
		free(CheatCodes);
	}

	CheatCodes = NULL;
	NumCodes = 0;
	NumCodesAllocated = 0;
}

// load cheats from the specific filename
void LoadCheats(const char *filename) {
	FILE				*fp;
	char				buf[256];
	int					count = 0;
	unsigned int		t1, t2;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		return;
	}

	ClearAllCheats();

	while (fgets(buf, 255, fp) != NULL) {
		buf[255] = '\0';
		trim(buf);

		// Skip comment or blank lines
		if (buf[0] == '#' || buf[0] == ';' || buf[0] == '/' || buf[0] == '\"' || buf[0] == '\0')
			continue;

		if (buf[0] == '[' && buf[strlen(buf) - 1] == ']') {
			if (NumCheats > 0)
				Cheats[NumCheats - 1].n = count;

			if (NumCheats >= NumCheatsAllocated) {
				NumCheatsAllocated += ALLOC_INCREMENT;

				if (Cheats == NULL) {
					assert(NumCheats == 0);
					assert(NumCheatsAllocated == ALLOC_INCREMENT);
					Cheats = (Cheat *)malloc(sizeof(Cheat) * NumCheatsAllocated);
				} else {
					Cheats = (Cheat *)realloc(Cheats, sizeof(Cheat) * NumCheatsAllocated);
				}
			}

			buf[strlen(buf) - 1] = '\0';
			count = 0;

			if (buf[1] == '*') {
				Cheats[NumCheats].Descr = strdup(buf + 2);
				Cheats[NumCheats].Enabled = 1;
			} else {
				Cheats[NumCheats].Descr = strdup(buf + 1);
				Cheats[NumCheats].Enabled = 0;
			}
			Cheats[NumCheats].WasEnabled = 0;

			Cheats[NumCheats].First = NumCodes;

			NumCheats++;
			continue;
		}

		if (NumCheats <= 0)
			continue;

		if (NumCodes >= NumCodesAllocated) {
			NumCodesAllocated += ALLOC_INCREMENT;

			if (CheatCodes == NULL) {
				assert(NumCodes == 0);
				assert(NumCodesAllocated == ALLOC_INCREMENT);
				CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
			} else {
				CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
			}
		}

		sscanf(buf, "%x %x", &t1, &t2);

		CheatCodes[NumCodes].Addr = t1;
		CheatCodes[NumCodes].Val = t2;

		NumCodes++;
		count++;
	}

	if (NumCheats > 0)
		Cheats[NumCheats - 1].n = count;

	fclose(fp);

	SysPrintf(_("Cheats loaded from: %s\n"), filename);
}

// save all cheats to the specified filename
void SaveCheats(const char *filename) {
	FILE		*fp;
	int			i, j;

	fp = fopen(filename, "w");
	if (fp == NULL) {
		return;
	}

	for (i = 0; i < NumCheats; i++) {
		// write the description
		if (Cheats[i].Enabled)
			fprintf(fp, "[*%s]\n", Cheats[i].Descr);
		else
			fprintf(fp, "[%s]\n", Cheats[i].Descr);

		// write all cheat codes
		for (j = 0; j < Cheats[i].n; j++) {
			fprintf(fp, "%.8X %.4X\n",
				CheatCodes[Cheats[i].First + j].Addr,
				CheatCodes[Cheats[i].First + j].Val);
		}

		fprintf(fp, "\n");
	}

	fclose(fp);

	SysPrintf(_("Cheats saved to: %s\n"), filename);
}

// apply all enabled cheats
void ApplyCheats() {
	int		i, j, k, endindex;
	int		was_enabled;

	for (i = 0; i < NumCheats; i++) {
		was_enabled = Cheats[i].WasEnabled;
		if (!Cheats[i].Enabled) {
			if (!Cheats[i].WasEnabled)
				continue;
		}
		Cheats[i].WasEnabled = Cheats[i].Enabled;

		// process all cheat codes
		endindex = Cheats[i].First + Cheats[i].n;

		for (j = Cheats[i].First; j < endindex; j++) {
			u8		type = (uint8_t)(CheatCodes[j].Addr >> 24);
			u32		addr = (CheatCodes[j].Addr & 0x001FFFFF);
			u16		val = CheatCodes[j].Val;
			u32		taddr;

			if (!was_enabled) {
				switch (type) {
					case CHEAT_CONST16:
						CheatCodes[j].OldVal = psxMu16(addr);
						break;
					case CHEAT_CONST8:
						CheatCodes[j].OldVal = psxMu8(addr);
						break;
				}
			}
			else if (!Cheats[i].Enabled) {
				val = CheatCodes[j].OldVal;
				if (type != CHEAT_CONST16 && type != CHEAT_CONST8)
					continue;
			}

			switch (type) {
				case CHEAT_CONST8:
					psxMu8ref(addr) = (u8)val;
					break;

				case CHEAT_CONST16:
					psxMu16ref(addr) = SWAPu16(val);
					break;

				case CHEAT_SCRATCHPAD16: // 1F
					psxHs16ref(addr) = SWAPu16(val);
					break;

				case CHEAT_INC16:
					psxMu16ref(addr) = SWAPu16(psxMu16(addr) + val);
					break;

				case CHEAT_DEC16:
					psxMu16ref(addr) = SWAPu16(psxMu16(addr) - val);
					break;

				case CHEAT_INC8:
					psxMu8ref(addr) += (u8)val;
					break;

				case CHEAT_DEC8:
					psxMu8ref(addr) -= (u8)val;
					break;

				case CHEAT_SLIDE:
					j++;
					if (j >= endindex)
						break;

					type = (uint8_t)(CheatCodes[j].Addr >> 24);
					taddr = (CheatCodes[j].Addr & 0x001FFFFF);
					val = CheatCodes[j].Val;

					if (type == CHEAT_CONST8) {
						for (k = 0; k < ((addr >> 8) & 0xFF); k++) {
							psxMu8ref(taddr) = (u8)val;
							taddr += (s8)(addr & 0xFF);
							val += (s8)(CheatCodes[j - 1].Val & 0xFF);
						}
					}
					else if (type == CHEAT_CONST16) {
						for (k = 0; k < ((addr >> 8) & 0xFF); k++) {
							psxMu16ref(taddr) = SWAPu16(val);
							taddr += (s8)(addr & 0xFF);
							val += (s8)(CheatCodes[j - 1].Val & 0xFF);
						}
					}
					break;

				case CHEAT_MEMCPY:
					j++;
					if (j >= endindex)
						break;

					taddr = (CheatCodes[j].Addr & 0x001FFFFF);
					for (k = 0; k < val; k++) {
						psxMu8ref(taddr + k) = PSXMu8(addr + k);
					}
					break;

				case CHEAT_EQU8:
					if (PSXMu8(addr) != (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_NOTEQU8:
					if (PSXMu8(addr) == (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_LESSTHAN8:
					if (PSXMu8(addr) >= (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_GREATERTHAN8:
					if (PSXMu8(addr) <= (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_EQU16:
					if (PSXMu16(addr) != val)
						j++; // skip the next code
					break;

				case CHEAT_NOTEQU16:
					if (PSXMu16(addr) == val)
						j++; // skip the next code
					break;

				case CHEAT_LESSTHAN16:
					if (PSXMu16(addr) >= val)
						j++; // skip the next code
					break;

				case CHEAT_GREATERTHAN16:
					if (PSXMu16(addr) <= val)
						j++; // skip the next code
					break;

				case CHEAT_BUTTONS1_16: { // D4
					u16 keys = in_keystate[0];
					keys = (keys << 8) | (keys >> 8);
					if (keys != val)
						j++; // skip the next code
					break;
				}

				default:
					SysPrintf("unhandled cheat %d,%d code %08X\n",
						i, j, CheatCodes[j].Addr);
					Cheats[i].WasEnabled = Cheats[i].Enabled = 0;
					break;
			}
		}
	}
}

int AddCheat(const char *descr, char *code) {
	int c = 1;
	char *p1, *p2;

	if (NumCheats >= NumCheatsAllocated) {
		NumCheatsAllocated += ALLOC_INCREMENT;

		if (Cheats == NULL) {
			assert(NumCheats == 0);
			assert(NumCheatsAllocated == ALLOC_INCREMENT);
			Cheats = (Cheat *)malloc(sizeof(Cheat) * NumCheatsAllocated);
		}
		else {
			Cheats = (Cheat *)realloc(Cheats, sizeof(Cheat) * NumCheatsAllocated);
		}
	}

	Cheats[NumCheats].Enabled = 0;
	Cheats[NumCheats].WasEnabled = 0;
	Cheats[NumCheats].First = NumCodes;
	Cheats[NumCheats].n = 0;

	p1 = code;
	p2 = code;

	while (c) {
		unsigned int t1, t2, r;

		while (*p2 != '\n' && *p2 != '\0')
			p2++;

		if (*p2 == '\0')
			c = 0;

		*p2 = '\0';
		p2++;

		t1 = 0;
		t2 = 0;
		r = sscanf(p1, "%x %x", &t1, &t2);

		if (r != 2)
			SysPrintf("cheat %d: couldn't parse '%s'\n", NumCodes, p1);
		else if (t1 >= 0x10000000) {
			if (NumCodes >= NumCodesAllocated) {
				NumCodesAllocated += ALLOC_INCREMENT;

				if (CheatCodes == NULL) {
					assert(NumCodes == 0);
					assert(NumCodesAllocated == ALLOC_INCREMENT);
					CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
				}
				else {
					CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
				}
			}

			CheatCodes[NumCodes].Addr = t1;
			CheatCodes[NumCodes].Val = t2;
			NumCodes++;
			Cheats[NumCheats].n++;
		}

		p1 = p2;
	}

	if (Cheats[NumCheats].n == 0) {
		return -1;
	}

	Cheats[NumCheats].Descr = strdup(descr[0] ? descr : _("(Untitled)"));
	NumCheats++;
	return 0;
}

void RemoveCheat(int index) {
	assert(index >= 0 && index < NumCheats);

	free(Cheats[index].Descr);
	Cheats[index].Descr = NULL;

	while (index < NumCheats - 1) {
		Cheats[index] = Cheats[index + 1];
		index++;
	}

	NumCheats--;
}

int EditCheat(int index, const char *descr, char *code) {
	int c = 1;
	int prev = NumCodes;
	char *p1, *p2;

	assert(index >= 0 && index < NumCheats);

	p1 = code;
	p2 = code;

	while (c) {
		unsigned int t1, t2;

		while (*p2 != '\n' && *p2 != '\0')
			p2++;

		if (*p2 == '\0')
			c = 0;

		*p2 = '\0';
		p2++;

		t1 = 0;
		t2 = 0;
		sscanf(p1, "%x %x", &t1, &t2);

		if (t1 > 0x10000000) {
			if (NumCodes >= NumCodesAllocated) {
				NumCodesAllocated += ALLOC_INCREMENT;

				if (CheatCodes == NULL) {
					assert(NumCodes == 0);
					assert(NumCodesAllocated == ALLOC_INCREMENT);
					CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
				}
				else {
					CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
				}
			}

			CheatCodes[NumCodes].Addr = t1;
			CheatCodes[NumCodes].Val = t2;
			NumCodes++;
		}

		p1 = p2;
	}

	if (NumCodes == prev) {
		return -1;
	}

	free(Cheats[index].Descr);
	Cheats[index].Descr = strdup(descr[0] ? descr : _("(Untitled)"));
	Cheats[index].First = prev;
	Cheats[index].n = NumCodes - prev;

	return 0;
}

void FreeCheatSearchResults() {
	if (SearchResults != NULL) {
		free(SearchResults);
	}
	SearchResults = NULL;

	NumSearchResults = 0;
	NumSearchResultsAllocated = 0;
}

void FreeCheatSearchMem() {
	if (prevM != NULL) {
		free(prevM);
	}
	prevM = NULL;
}

void CheatSearchBackupMemory() {
	if (prevM != NULL) {
		memcpy(prevM, psxM, 0x200000);
	}
}

static void CheatSearchInitBackupMemory() {
	if (prevM == NULL) {
		prevM = (s8 *)malloc(0x200000);
		CheatSearchBackupMemory();
	}
}

static void CheatSearchAddResult(u32 addr) {
	if (NumSearchResults >= NumSearchResultsAllocated) {
		NumSearchResultsAllocated += ALLOC_INCREMENT;

		if (SearchResults == NULL) {
			SearchResults = (u32 *)malloc(sizeof(u32) * NumSearchResultsAllocated);
		}
		else {
			SearchResults = (u32 *)realloc(SearchResults, sizeof(u32) * NumSearchResultsAllocated);
		}
	}

	SearchResults[NumSearchResults++] = addr;
}

void CheatSearchEqual8(u8 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i++) {
			if (PSXMu8(i) == val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu8(SearchResults[i]) == val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchEqual16(u16 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 2) {
			if (PSXMu16(i) == val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu16(SearchResults[i]) == val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchEqual32(u32 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 4) {
			if (PSXMu32(i) == val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu32(SearchResults[i]) == val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchNotEqual8(u8 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i++) {
			if (PSXMu8(i) != val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu8(SearchResults[i]) != val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchNotEqual16(u16 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 2) {
			if (PSXMu16(i) != val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu16(SearchResults[i]) != val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchNotEqual32(u32 val) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 4) {
			if (PSXMu32(i) != val) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu32(SearchResults[i]) != val) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchRange8(u8 min, u8 max) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i++) {
			if (PSXMu8(i) >= min && PSXMu8(i) <= max) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu8(SearchResults[i]) >= min && PSXMu8(SearchResults[i]) <= max) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchRange16(u16 min, u16 max) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 2) {
			if (PSXMu16(i) >= min && PSXMu16(i) <= max) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu16(SearchResults[i]) >= min && PSXMu16(SearchResults[i]) <= max) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchRange32(u32 min, u32 max) {
	u32 i, j;

	CheatSearchInitBackupMemory();

	if (SearchResults == NULL) {
		// search the whole memory
		for (i = 0; i < 0x200000; i += 4) {
			if (PSXMu32(i) >= min && PSXMu32(i) <= max) {
				CheatSearchAddResult(i);
			}
		}
	}
	else {
		// only search within the previous results
		j = 0;

		for (i = 0; i < NumSearchResults; i++) {
			if (PSXMu32(SearchResults[i]) >= min && PSXMu32(SearchResults[i]) <= max) {
				SearchResults[j++] = SearchResults[i];
			}
		}

		NumSearchResults = j;
	}
}

void CheatSearchIncreasedBy8(u8 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PSXMu8(SearchResults[i]) - PrevMu8(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchIncreasedBy16(u16 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PSXMu16(SearchResults[i]) - PrevMu16(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchIncreasedBy32(u32 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PSXMu32(SearchResults[i]) - PrevMu32(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreasedBy8(u8 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu8(SearchResults[i]) - PSXMu8(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreasedBy16(u16 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu16(SearchResults[i]) - PSXMu16(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreasedBy32(u32 val) {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu32(SearchResults[i]) - PSXMu32(SearchResults[i]) == val) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchIncreased8() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu8(SearchResults[i]) < PSXMu8(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchIncreased16() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu16(SearchResults[i]) < PSXMu16(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchIncreased32() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu32(SearchResults[i]) < PSXMu32(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreased8() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu8(SearchResults[i]) > PSXMu8(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreased16() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu16(SearchResults[i]) > PSXMu16(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDecreased32() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu32(SearchResults[i]) > PSXMu32(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDifferent8() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu8(SearchResults[i]) != PSXMu8(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDifferent16() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu16(SearchResults[i]) != PSXMu16(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchDifferent32() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu32(SearchResults[i]) != PSXMu32(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchNoChange8() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu8(SearchResults[i]) == PSXMu8(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchNoChange16() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu16(SearchResults[i]) == PSXMu16(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}

void CheatSearchNoChange32() {
	u32 i, j;

	assert(prevM != NULL); // not possible for the first search

	j = 0;

	for (i = 0; i < NumSearchResults; i++) {
		if (PrevMu32(SearchResults[i]) == PSXMu32(SearchResults[i])) {
			SearchResults[j++] = SearchResults[i];
		}
	}

	NumSearchResults = j;
}
