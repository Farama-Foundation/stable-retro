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

#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "misc.h"
#include "sio.h"

extern char *biosA0n[256];
extern char *biosB0n[256];
extern char *biosC0n[256];

void psxBiosInit();
void psxBiosShutdown();
void psxBiosException();
void psxBiosFreeze(int Mode);
void psxBiosCnfLoaded(u32 tcb_cnt, u32 evcb_cnt, u32 sp);
void psxBiosSetupBootState(void);
void psxBiosCheckExe(u32 t_addr, u32 t_size, int loading_state);
void psxBiosCheckBranch(void);
int  psxBiosSoftcallEnded(void);

extern void (*biosA0[256])();
extern void (**biosB0)();
extern void (*biosC0[256+128])();

#ifdef __cplusplus
}
#endif
#endif
