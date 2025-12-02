/*  PPF Patch Support for PCSX-Reloaded
 *  Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 *  Based on P.E.Op.S CDR Plugin by Pete Bernert.
 *  Copyright (c) 2002, Pete Bernert.
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
#include "ppf.h"
#include "misc.h"
#include "cdrom.h"

typedef struct tagPPF_DATA {
	s32					addr;
	s32					pos;
	s32					anz;
	struct tagPPF_DATA	*pNext;
} PPF_DATA;

typedef struct tagPPF_CACHE {
	s32					addr;
	struct tagPPF_DATA	*pNext;
} PPF_CACHE;

static PPF_CACHE		*ppfCache = NULL;
static PPF_DATA			*ppfHead = NULL, *ppfLast = NULL;
static int				iPPFNum = 0;

// using a linked data list, and address array
static void FillPPFCache() {
	PPF_DATA		*p;
	PPF_CACHE		*pc;
	s32				lastaddr;

	p = ppfHead;
	lastaddr = -1;
	iPPFNum = 0;

	while (p != NULL) {
		if (p->addr != lastaddr) iPPFNum++;
		lastaddr = p->addr;
		p = p->pNext;
	}

	if (iPPFNum <= 0) return;

	pc = ppfCache = (PPF_CACHE *)malloc(iPPFNum * sizeof(PPF_CACHE));
	if (pc == NULL) return;

	iPPFNum--;
	p = ppfHead;
	lastaddr = -1;

	while (p != NULL) {
		if (p->addr != lastaddr) {
			pc->addr = p->addr;
			pc->pNext = p;
			pc++;
		}
		lastaddr = p->addr;
		p = p->pNext;
	}
}

void FreePPFCache() {
	PPF_DATA *p = ppfHead;
	void *pn;

	while (p != NULL) {
		pn = p->pNext;
		free(p);
		p = (PPF_DATA *)pn;
	}
	ppfHead = NULL;
	ppfLast = NULL;

	if (ppfCache != NULL) free(ppfCache);
	ppfCache = NULL;
}

void CheckPPFCache(unsigned char *pB, unsigned char m, unsigned char s, unsigned char f) {
	PPF_CACHE *pcstart, *pcend, *pcpos;
	int addr = MSF2SECT(m, s, f), pos, anz, start;

	if (ppfCache == NULL) return;

	pcstart = ppfCache;
	if (addr < pcstart->addr) return;
	pcend = ppfCache + iPPFNum;
	if (addr > pcend->addr) return;

	while (1) {
		if (addr == pcend->addr) { pcpos = pcend; break; }

		pcpos = pcstart + (pcend - pcstart) / 2;
		if (pcpos == pcstart) break;
		if (addr < pcpos->addr) {
			pcend = pcpos;
			continue;
		}
		if (addr > pcpos->addr) {
			pcstart = pcpos;
			continue;
		}
		break;
	}

	if (addr == pcpos->addr) {
		PPF_DATA *p = pcpos->pNext;
		while (p != NULL && p->addr == addr) {
			pos = p->pos - (CD_FRAMESIZE_RAW - DATA_SIZE);
			anz = p->anz;
			if (pos < 0) { start = -pos; pos = 0; anz -= start; }
			else start = 0;
			memcpy(pB + pos, (unsigned char *)(p + 1) + start, anz);
			p = p->pNext;
		}
	}
}

static void AddToPPF(s32 ladr, s32 pos, s32 anz, unsigned char *ppfmem) {
	if (ppfHead == NULL) {
		ppfHead = (PPF_DATA *)malloc(sizeof(PPF_DATA) + anz);
		if (ppfHead == NULL) return;
		ppfHead->addr = ladr;
		ppfHead->pNext = NULL;
		ppfHead->pos = pos;
		ppfHead->anz = anz;
		memcpy(ppfHead + 1, ppfmem, anz);
		iPPFNum = 1;
		ppfLast = ppfHead;
	} else {
		PPF_DATA *p = ppfHead;
		PPF_DATA *plast = NULL;
		PPF_DATA *padd;

		if (ladr > ppfLast->addr || (ladr == ppfLast->addr && pos > ppfLast->pos)) {
			p = NULL;
			plast = ppfLast;
		} else {
			while (p != NULL) {
				if (ladr < p->addr) break;
				if (ladr == p->addr) {
					while (p && ladr == p->addr && pos > p->pos) {
						plast = p;
						p = p->pNext;
					}
					break;
				}
				plast = p;
				p = p->pNext;
			}
		}

		padd = (PPF_DATA *)malloc(sizeof(PPF_DATA) + anz);
		if (padd == NULL) return;
		padd->addr = ladr;
		padd->pNext = p;
		padd->pos = pos;
		padd->anz = anz;
		memcpy(padd + 1, ppfmem, anz);
		iPPFNum++;
		if (plast == NULL) ppfHead = padd;
		else plast->pNext = padd;

		if (padd->pNext == NULL) ppfLast = padd;
	}
}

void BuildPPFCache(const char *fname) {
	FILE			*ppffile;
	char			buffer[12];
	char			method, undo = 0, blockcheck = 0;
	int				dizlen, dizyn;
	unsigned char	ppfmem[512];
	char			szPPF[MAXPATHLEN * 2];
	int				count, seekpos, pos;
	u32				anz; // use 32-bit to avoid stupid overflows
	s32				ladr, off, anx;

	FreePPFCache();

	if (CdromId[0] == '\0') return;

	if (!fname) {
		// Generate filename in the format of SLUS_123.45
		buffer[0] = toupper(CdromId[0]);
		buffer[1] = toupper(CdromId[1]);
		buffer[2] = toupper(CdromId[2]);
		buffer[3] = toupper(CdromId[3]);
		buffer[4] = '_';
		buffer[5] = CdromId[4];
		buffer[6] = CdromId[5];
		buffer[7] = CdromId[6];
		buffer[8] = '.';
		buffer[9] = CdromId[7];
		buffer[10] = CdromId[8];
		buffer[11] = '\0';

		sprintf(szPPF, "%s%s", Config.PatchesDir, buffer);
		fname = szPPF;
	}
	ppffile = fopen(fname, "rb");
	if (ppffile == NULL) return;

	memset(buffer, 0, 5);
	if (fread(buffer, 1, 3, ppffile) != 3)
		goto fail_io;

	if (strcmp(buffer, "PPF") != 0) {
		SysPrintf(_("Invalid PPF patch: %s.\n"), fname);
		fclose(ppffile);
		return;
	}

	fseek(ppffile, 5, SEEK_SET);
	method = fgetc(ppffile);

	switch (method) {
		case 0: // ppf1
			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);
			count -= 56;
			seekpos = 56;
			break;

		case 1: // ppf2
			fseek(ppffile, -8, SEEK_END);

			memset(buffer, 0, 5);
			if (fread(buffer, 1, 4, ppffile) != 4)
				goto fail_io;

			if (strcmp(".DIZ", buffer) != 0) {
				dizyn = 0;
			} else {
				if (fread(&dizlen, 1, 4, ppffile) != 4)
					goto fail_io;
				dizlen = SWAP32(dizlen);
				dizyn = 1;
			}

			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);

			if (dizyn == 0) {
				count -= 1084;
				seekpos = 1084;
			} else {
				count -= 1084;
				count -= 38;
				count -= dizlen;
				seekpos = 1084;
			}
			break;

		case 2: // ppf3
			fseek(ppffile, 57, SEEK_SET);
			blockcheck = fgetc(ppffile);
			undo = fgetc(ppffile);

			fseek(ppffile, -6, SEEK_END);
			memset(buffer, 0, 5);
			if (fread(buffer, 1, 4, ppffile) != 4)
				goto fail_io;
			dizlen = 0;

			if (strcmp(".DIZ", buffer) == 0) {
				fseek(ppffile, -2, SEEK_END);
				// TODO: Endian/size unsafe?
				if (fread(&dizlen, 1, 2, ppffile) != 2)
					goto fail_io;
				dizlen = SWAP32(dizlen);
				dizlen += 36;
			}

			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);
			count -= dizlen;

			if (blockcheck) {
				seekpos = 1084;
				count -= 1084;
			} else {
				seekpos = 60;
				count -= 60;
			}
			break;

		default:
			fclose(ppffile);
			SysPrintf(_("Unsupported PPF version (%d).\n"), method + 1);
			return;
	}

	// now do the data reading
	do {                                                
		fseek(ppffile, seekpos, SEEK_SET);
		if (fread(&pos, 1, sizeof(pos), ppffile) != sizeof(pos))
			goto fail_io;
		pos = SWAP32(pos);

		if (method == 2) {
			// skip 4 bytes on ppf3 (no int64 support here)
			if (fread(buffer, 1, 4, ppffile) != 4)
				goto fail_io;
		}

		anz = fgetc(ppffile);
		if (fread(ppfmem, 1, anz, ppffile) != anz)
			goto fail_io;

		ladr = pos / CD_FRAMESIZE_RAW;
		off = pos % CD_FRAMESIZE_RAW;

		if (off + anz > CD_FRAMESIZE_RAW) {
			anx = off + anz - CD_FRAMESIZE_RAW;
			anz -= (unsigned char)anx;
			AddToPPF(ladr + 1, 0, anx, &ppfmem[anz]);
		}

		AddToPPF(ladr, off, anz, ppfmem); // add to link list

		if (method == 2) {
			if (undo) anz += anz;
			anz += 4;
		}

		seekpos = seekpos + 5 + anz;
		count = count - 5 - anz;
	} while (count != 0); // loop til end

	fclose(ppffile);

	FillPPFCache(); // build address array

	SysPrintf(_("Loaded PPF %d.0 patch: %s.\n"), method + 1, fname);
	return;

fail_io:
#ifndef NDEBUG
	SysPrintf(_("File IO error in <%s:%s>.\n"), __FILE__, __func__);
#endif
	fclose(ppffile);
}

// redump.org SBI files, slightly different handling from PCSX-Reloaded
unsigned char *sbi_sectors;
int sbi_len;

int LoadSBI(const char *fname, int sector_count) {
	int good_sectors = 0;
	int clean_eof = 0;
	char buffer[16];
	FILE *sbihandle;
	u8 sbitime[3], t;
	int s;

	sbihandle = fopen(fname, "rb");
	if (sbihandle == NULL)
		return -1;

	sbi_len = (sector_count + 7) / 8;
	sbi_sectors = calloc(1, sbi_len);
	if (sbi_sectors == NULL)
		goto end;

	// 4-byte SBI header
	if (fread(buffer, 1, 4, sbihandle) != 4)
		goto end;

	while (1) {
		s = fread(sbitime, 1, 3, sbihandle);
		if (s != 3)
		{
			if (s == 0)
				clean_eof = 1;
			break;
		}
		s = MSF2SECT(btoi(sbitime[0]), btoi(sbitime[1]), btoi(sbitime[2]));
		if (s < sector_count) {
			sbi_sectors[s >> 3] |= 1 << (s&7);
			good_sectors++;
		}
		else
			SysPrintf(_("SBI sector %d >= %d?\n"), s, sector_count);

		// skip to the next record
		if (fread(&t, 1, sizeof(t), sbihandle) != sizeof(t))
			break;
		s = -1;
		switch (t) {
		default:
		case 1:
			s = 10;
			break;
		case 2:
		case 3:
			s = 3;
			break;
		}
		if (s < 0)
			break;
		if (fseek(sbihandle, s, SEEK_CUR))
			break;
	}

end:
	if (!clean_eof)
		SysPrintf(_("SBI: parse failure at 0x%lx\n"), ftell(sbihandle));
	if (!good_sectors) {
		free(sbi_sectors);
		sbi_sectors = NULL;
		sbi_len = 0;
	}
	fclose(sbihandle);
	return sbi_sectors ? 0 : -1;
}

void UnloadSBI(void) {
	if (sbi_sectors) {
		free(sbi_sectors);
		sbi_sectors = NULL;
		sbi_len = 0;
	}
}
