/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - dd_disk.c                                               *
*   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
*   Copyright (C) 2015 LuigiBlood                                         *
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

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dd_controller.h"
#include "dd_disk.h"
#include "pi/pi_controller.h"

#include "api/callbacks.h"
#include "api/config.h"
#include "api/m64p_config.h"
#include "api/m64p_types.h"
#include "main/main.h"
#include "main/rom.h"
#include "main/util.h"
#include "r4300/cp0.h"
#include "r4300/cp0_private.h"
#include "r4300/r4300_core.h"

/* Global loaded disk memory space. */
unsigned char* g_dd_disk = NULL;
/* Global loaded disk size. */
int g_dd_disk_size = 0;

int dd_bm_mode_read;	/* BM MODE 0 = WRITE, MODE 1 = READ */
int CUR_BLOCK;			/* Current Block */
int dd_zone;			/* Current Zone */
int dd_track_offset;	/* Offset to Track */
int disk_format;		/* Disk Dump Format */

const unsigned int ddZoneSecSize[16] = { 232, 216, 208, 192, 176, 160, 144, 128,
                                       216, 208, 192, 176, 160, 144, 128, 112 };

const unsigned int ddZoneTrackSize[16] = { 158, 158, 149, 149, 149, 149, 149, 114,
                                         158, 158, 149, 149, 149, 149, 149, 114 };

const unsigned int ddStartOffset[16] =
            { 0x0, 0x5F15E0, 0xB79D00, 0x10801A0, 0x1523720, 0x1963D80, 0x1D414C0, 0x20BBCE0,
            0x23196E0, 0x28A1E00, 0x2DF5DC0, 0x3299340, 0x36D99A0, 0x3AB70E0, 0x3E31900, 0x4149200 };

void init_dd_disk(struct dd_disk* dd_disk,
	                 uint8_t* disk, size_t disk_size)
{
	dd_disk->disk = disk;
	dd_disk->disk_size = disk_size;
}

void format_disk(uint8_t* disk)
{
	memset(disk, 0, MAME_FORMAT_DUMP_SIZE);
}

m64p_error open_dd_disk(const unsigned char* diskimage, unsigned int size)
{
	/* allocate new buffer for ROM and copy into this buffer */
	if (size == MAME_FORMAT_DUMP_SIZE)
	{
		DebugMessage(M64MSG_STATUS, "64DD Disk Format: MAME");
		disk_format = MAME_FORMAT_DUMP;

		g_dd_disk_size = size;
		/* g_dd_disk = (unsigned char *)malloc(size); */
		if (g_dd_disk == NULL)
			return M64ERR_NO_MEMORY;

		/* Load Disk only if it's not saved */
		if (*((uint32_t *)g_dd_disk) != 0x16D348E8 && *((uint32_t *)g_dd_disk) != 0x56EE6322)
			memcpy(g_dd_disk, diskimage, size);
	}
	else if (size == SDK_FORMAT_DUMP_SIZE)
	{
		DebugMessage(M64MSG_STATUS, "64DD Disk Format: SDK");
		disk_format = SDK_FORMAT_DUMP;

		g_dd_disk_size = MAME_FORMAT_DUMP_SIZE;
		/* g_dd_disk = (unsigned char *)malloc(MAME_FORMAT_DUMP_SIZE); */
		if (g_dd_disk == NULL)
			return M64ERR_NO_MEMORY;

		/* CONVERSION NEEDED INTERNALLY */
		/* Load Disk only if it's not saved */
		if (*((uint32_t *)g_dd_disk) != 0x16D348E8 && *((uint32_t *)g_dd_disk) != 0x56EE6322)
			dd_convert_to_mame(diskimage);
	}
	else
	{
		DebugMessage(M64MSG_STATUS, "64DD Disk Format: Unknown, don't load.");
		return M64ERR_FILES;
	}

	DebugMessage(M64MSG_STATUS, "64DD Disk loaded!");

	return M64ERR_SUCCESS;
}

m64p_error close_dd_disk(void)
{
	if (g_dd_disk == NULL)
		return M64ERR_INVALID_STATE;
	
#if 0
	free(g_dd_disk);
	g_dd_disk = NULL;
#endif

	DebugMessage(M64MSG_STATUS, "64DD Disk closed.");

	return M64ERR_SUCCESS;
}

void dd_set_zone_and_track_offset(void *opaque)
{
	int tr_off;
	struct dd_controller *dd = (struct dd_controller *) opaque;

	int head = ((dd->regs[ASIC_CUR_TK] & 0x10000000U) >> 25);	/* Head * 8 */
	int track = ((dd->regs[ASIC_CUR_TK] & 0x0FFF0000U) >> 16);

	if (track >= 0x425)
	{
		dd_zone = 7 + head;
		tr_off = track - 0x425;
	}
	else if (track >= 0x390)
	{
		dd_zone = 6 + head;
		tr_off = track - 0x390;
	}
	else if (track >= 0x2FB)
	{
		dd_zone = 5 + head;
		tr_off = track - 0x2FB;
	}
	else if (track >= 0x266)
	{
		dd_zone = 4 + head;
		tr_off = track - 0x266;
	}
	else if (track >= 0x1D1)
	{
		dd_zone = 3 + head;
		tr_off = track - 0x1D1;
	}
	else if (track >= 0x13C)
	{
		dd_zone = 2 + head;
		tr_off = track - 0x13C;
	}
	else if (track >= 0x9E)
	{
		dd_zone = 1 + head;
		tr_off = track - 0x9E;
	}
	else
	{
		dd_zone = 0 + head;
		tr_off = track;
	}

	dd_track_offset = ddStartOffset[dd_zone] + tr_off*ddZoneSecSize[dd_zone] * SECTORS_PER_BLOCK*BLOCKS_PER_TRACK;
}

void dd_update_bm(void *opaque)
{
	struct dd_controller *dd = (struct dd_controller *) opaque;

	if ((dd->regs[ASIC_BM_STATUS_CTL] & 0x80000000) == 0)
		return;
	else
	{
		int Cur_Sector = dd->regs[ASIC_CUR_SECTOR] >> 16;
		if (Cur_Sector >= 0x5A)
		{
			CUR_BLOCK = 1;
			Cur_Sector -= 0x5A;
		}
		else
		{
			CUR_BLOCK = 0;
		}

		if (!dd_bm_mode_read)		/* WRITE MODE */
		{
#if 0
			printf("--DD_UPDATE_BM WRITE Block %d Sector %X\n",
               ((dd->regs[ASIC_CUR_TK] & 0x1FFF0000U) >> 15) + CUR_BLOCK, Cur_Sector);
#endif

			if (Cur_Sector < SECTORS_PER_BLOCK)
			{
				dd_write_sector(dd);
				Cur_Sector++;
				dd->regs[ASIC_CMD_STATUS] |= 0x40000000;
			}
			else if (Cur_Sector < SECTORS_PER_BLOCK + 1)
			{
				if (dd->regs[ASIC_BM_STATUS_CTL] & 0x01000000)
				{
					CUR_BLOCK = 1 - CUR_BLOCK;
					Cur_Sector = 0;
					dd_write_sector(dd);

					/* next block */
					Cur_Sector += 1;
					dd->regs[ASIC_BM_STATUS_CTL] &= ~0x01000000;
					dd->regs[ASIC_CMD_STATUS] |= 0x40000000;
				}
				else
				{
					Cur_Sector++;
					dd->regs[ASIC_BM_STATUS_CTL] &= ~0x80000000;
				}
			}
		}
		else						/* READ MODE */
		{
			int Cur_Track = (dd->regs[ASIC_CUR_TK] & 0x1FFF0000U) >> 16;

#if 0
			printf("--DD_UPDATE_BM READ Block %d Sector %X\n",
               ((dd->regs[ASIC_CUR_TK] & 0x1FFF0000U) >> 15) + CUR_BLOCK, Cur_Sector);
#endif

			if (Cur_Track == 6 && CUR_BLOCK == 0)
			{
				dd->regs[ASIC_CMD_STATUS] &= ~0x40000000;
				dd->regs[ASIC_BM_STATUS_CTL] |= 0x02000000;
			}
			else if (Cur_Sector < SECTORS_PER_BLOCK)		/* user sector */
			{
				dd_read_sector(dd);
				Cur_Sector++;
				dd->regs[ASIC_CMD_STATUS] |= 0x40000000;
			}
			else if (Cur_Sector < SECTORS_PER_BLOCK + 4)	/* C2 */
			{
				Cur_Sector++;
				if (Cur_Sector == SECTORS_PER_BLOCK + 4)
				{
#if 0
					dd_read_C2(opaque);
#endif
					dd->regs[ASIC_CMD_STATUS] |= 0x10000000;
				}
			}
			else if (Cur_Sector == SECTORS_PER_BLOCK + 4)	/* Gap */
			{
				if (dd->regs[ASIC_BM_STATUS_CTL] & 0x01000000)
				{
					CUR_BLOCK = 1 - CUR_BLOCK;
					Cur_Sector = 0;
					dd->regs[ASIC_BM_STATUS_CTL] &= ~0x01000000;
				}
				else
					dd->regs[ASIC_BM_STATUS_CTL] &= ~0x80000000;
			}
		}

		dd->regs[ASIC_CUR_SECTOR] = (Cur_Sector + (0x5A * CUR_BLOCK)) << 16;

		dd->regs[ASIC_CMD_STATUS] |= 0x04000000;
		cp0_update_count();
		g_cp0_regs[CP0_CAUSE_REG] |= 0x00000800;
        check_interrupt();
	}
}

void dd_write_sector(void *opaque)
{
   unsigned i;
   int Cur_Sector, offset;
	struct dd_controller *dd = (struct dd_controller *) opaque;

	/* WRITE SECTOR */
	Cur_Sector = dd->regs[ASIC_CUR_SECTOR] >> 16;
	if (Cur_Sector >= 0x5A)
		Cur_Sector -= 0x5A;

	offset = dd_track_offset;
	offset += CUR_BLOCK * SECTORS_PER_BLOCK * ddZoneSecSize[dd_zone];
	offset += (Cur_Sector - 1) * ddZoneSecSize[dd_zone];

	for (i = 0; i <= (int)(dd->regs[ASIC_HOST_SECBYTE] >> 16); i++)
		g_dd_disk[offset + i] = dd->sec_buf[i ^ 3];
}

void dd_read_sector(void *opaque)
{
   unsigned i;
   int offset, Cur_Sector;
	struct dd_controller *dd = (struct dd_controller *) opaque;

	/* READ SECTOR */
	Cur_Sector = dd->regs[ASIC_CUR_SECTOR] >> 16;
	if (Cur_Sector >= 0x5A)
		Cur_Sector -= 0x5A;

	offset = dd_track_offset;
	offset += CUR_BLOCK * SECTORS_PER_BLOCK * ddZoneSecSize[dd_zone];
	offset += Cur_Sector * ddZoneSecSize[dd_zone];

	for (i = 0; i <= (int)(dd->regs[ASIC_HOST_SECBYTE] >> 16); i++)
		dd->sec_buf[i ^ 3] = g_dd_disk[offset + i];
}

/* CONVERSION */
void dd_convert_to_mame(const unsigned char* diskimage)
{
    /* Original code by Happy_ */
    const uint32_t ZoneSecSize[16] = { 232, 216, 208, 192, 176, 160, 144, 128,
        216, 208, 192, 176, 160, 144, 128, 112 };
    const uint32_t ZoneTracks[16] = { 158, 158, 149, 149, 149, 149, 149, 114,
        158, 158, 149, 149, 149, 149, 149, 114 };
    const uint32_t DiskTypeZones[7][16] = {
        { 0, 1, 2, 9, 8, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 10, 9, 8, 4, 5, 6, 7, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 11, 10, 9, 8, 5, 6, 7, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 12, 11, 10, 9, 8, 6, 7, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t RevDiskTypeZones[7][16] = {
        { 0, 1, 2, 5, 6, 7, 8, 9, 4, 3, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 7, 8, 9, 10, 6, 5, 4, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 9, 10, 11, 8, 7, 6, 5, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 11, 12, 10, 9, 8, 7, 6, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t StartBlock[7][16] = {
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1 }
    };

    uint32_t disktype = 0;
    uint32_t zone, track = 0;
    int32_t atrack = 0;
    int32_t block = 0;
    uint8_t SystemData[0xE8];
    uint8_t BlockData0[0x100 * SECTORS_PER_BLOCK];
    uint8_t BlockData1[0x100 * SECTORS_PER_BLOCK];
    uint32_t InOffset, OutOffset = 0;
    uint32_t InStart[16];
    uint32_t OutStart[16];

    int cur_offset = 0;

    InStart[0] = 0;
    OutStart[0] = 0;

    /* Read System Area */
    memcpy(SystemData, diskimage, 0xE8);

    disktype = SystemData[5] & 0xF;

    /* Prepare Input Offsets */
    for (zone = 1; zone < 16; zone++)
    {
        InStart[zone] = InStart[zone - 1] +
            VZONESIZE(DiskTypeZones[disktype][zone - 1]);
    }

    /* Prepare Output Offsets */
    for (zone = 1; zone < 16; zone++)
    {
        OutStart[zone] = OutStart[zone - 1] + ZONESIZE(zone - 1);
    }

    /* Copy Head 0 */
    for (zone = 0; zone < 8; zone++)
    {
        OutOffset = OutStart[zone];
        InOffset = InStart[RevDiskTypeZones[disktype][zone]];
        cur_offset = InOffset;
        block = StartBlock[disktype][zone];
        atrack = 0;
        for (track = 0; track < ZoneTracks[zone]; track++)
        {
            if (atrack < 0xC && track == SystemData[0x20 + zone * 0xC + atrack])
            {
                memset((void *)(&BlockData0), 0, BLOCKSIZE(zone));
                memset((void *)(&BlockData1), 0, BLOCKSIZE(zone));
                atrack += 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                	memcpy(BlockData1, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                    memcpy(BlockData0, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                }
                else
                {
                    memcpy(BlockData0, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                    memcpy(BlockData1, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                }
                block = 1 - block;
            }
            memcpy(g_dd_disk + OutOffset, &BlockData0, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
            memcpy(g_dd_disk + OutOffset, &BlockData1, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
        }
    }

    /* Copy Head 1 */
    for (zone = 8; zone < 16; zone++)
    {
        InOffset = InStart[RevDiskTypeZones[disktype][zone]];
        cur_offset = InOffset;
        block = StartBlock[disktype][zone];
        atrack = 0xB;
        for (track = 1; track < ZoneTracks[zone] + 1; track++)
        {
            if (atrack > -1 && (ZoneTracks[zone] - track) == SystemData[0x20 + (zone)* 0xC + atrack])
            {
                memset((void *)(&BlockData0), 0, BLOCKSIZE(zone));
                memset((void *)(&BlockData1), 0, BLOCKSIZE(zone));
                atrack -= 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                    memcpy(BlockData1, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                    memcpy(BlockData0, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                }
                else
                {
                    memcpy(BlockData0, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                    memcpy(BlockData1, diskimage + cur_offset, BLOCKSIZE(zone));
                    cur_offset += BLOCKSIZE(zone);
                }
                block = 1 - block;
            }
            OutOffset = OutStart[zone] + (ZoneTracks[zone] - track) * TRACKSIZE(zone);
            memcpy(g_dd_disk + OutOffset, &BlockData0, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
            memcpy(g_dd_disk + OutOffset, &BlockData1, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
        }
    }
}
