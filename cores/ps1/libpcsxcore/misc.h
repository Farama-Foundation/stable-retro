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

#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "coff.h"
#include "plugins.h"
#include "r3000a.h"
#include "psxmem.h"

#undef s_addr

typedef struct {
	unsigned char id[8];
    u32 text;                   
    u32 data;                    
    u32 pc0;
    u32 gp0;                     
    u32 t_addr;
    u32 t_size;
    u32 d_addr;                  
    u32 d_size;                  
    u32 b_addr;                  
    u32 b_size;                  
    u32 s_addr;
    u32 s_size;
    u32 SavedSP;
    u32 SavedFP;
    u32 SavedGP;
    u32 SavedRA;
    u32 SavedS0;
} EXE_HEADER;

extern char CdromId[10];
extern char CdromLabel[33];
extern int  CdromFrontendId; // for frontend use

int BiosBootBypass();

int LoadCdrom();
int LoadCdromFile(const char *filename, EXE_HEADER *head, u8 *time_bcd_out);
int CheckCdrom();
int Load(const char *ExePath);

int SaveState(const char *file);
int LoadState(const char *file);
int CheckState(const char *file);

void trim(char *str);
u16 calcCrc(const u8 *d, int len);

const char *get_build_info(void);

#ifdef __cplusplus
}
#endif
#endif
