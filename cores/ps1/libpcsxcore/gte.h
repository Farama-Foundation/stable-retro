/***************************************************************************
 *   PCSX-Revolution - PlayStation Emulator for Nintendo Wii               *
 *   Copyright (C) 2009-2010  PCSX-Revolution Dev Team                     *
 *   <http://code.google.com/p/pcsx-revolution/>                           *
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

#ifdef FLAGLESS

#define gteRTPS gteRTPS_nf
#define gteOP gteOP_nf
#define gteNCLIP gteNCLIP_nf
#define gteDPCS gteDPCS_nf
#define gteINTPL gteINTPL_nf
#define gteMVMVA gteMVMVA_nf
#define gteNCDS gteNCDS_nf
#define gteNCDT gteNCDT_nf
#define gteCDP gteCDP_nf
#define gteNCCS gteNCCS_nf
#define gteCC gteCC_nf
#define gteNCS gteNCS_nf
#define gteNCT gteNCT_nf
#define gteSQR gteSQR_nf
#define gteDCPL gteDCPL_nf
#define gteDPCT gteDPCT_nf
#define gteAVSZ3 gteAVSZ3_nf
#define gteAVSZ4 gteAVSZ4_nf
#define gteRTPT gteRTPT_nf
#define gteGPF gteGPF_nf
#define gteGPL gteGPL_nf
#define gteNCCT gteNCCT_nf

#define gteGPL_part_noshift gteGPL_part_noshift_nf
#define gteGPL_part_shift gteGPL_part_shift_nf
#define gteDPCS_part_noshift gteDPCS_part_noshift_nf
#define gteDPCS_part_shift gteDPCS_part_shift_nf
#define gteINTPL_part_noshift gteINTPL_part_noshift_nf
#define gteINTPL_part_shift gteINTPL_part_shift_nf
#define gteMACtoRGB gteMACtoRGB_nf

#undef __GTE_H__
#endif

#ifndef __GTE_H__
#define __GTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"

struct psxCP2Regs;

extern const unsigned char gte_cycletab[64];

void gteCheckStall(u32 op);

u32  MFC2(struct psxCP2Regs *regs, int reg);
void MTC2(struct psxCP2Regs *regs, u32 value, int reg);
void CTC2(struct psxCP2Regs *regs, u32 value, int reg);

void gteRTPS(struct psxCP2Regs *regs);
void gteOP(struct psxCP2Regs *regs);
void gteNCLIP(struct psxCP2Regs *regs);
void gteDPCS(struct psxCP2Regs *regs);
void gteINTPL(struct psxCP2Regs *regs);
void gteMVMVA(struct psxCP2Regs *regs);
void gteNCDS(struct psxCP2Regs *regs);
void gteNCDT(struct psxCP2Regs *regs);
void gteCDP(struct psxCP2Regs *regs);
void gteNCCS(struct psxCP2Regs *regs);
void gteCC(struct psxCP2Regs *regs);
void gteNCS(struct psxCP2Regs *regs);
void gteNCT(struct psxCP2Regs *regs);
void gteSQR(struct psxCP2Regs *regs);
void gteDCPL(struct psxCP2Regs *regs);
void gteDPCT(struct psxCP2Regs *regs);
void gteAVSZ3(struct psxCP2Regs *regs);
void gteAVSZ4(struct psxCP2Regs *regs);
void gteRTPT(struct psxCP2Regs *regs);
void gteGPF(struct psxCP2Regs *regs);
void gteGPL(struct psxCP2Regs *regs);
void gteNCCT(struct psxCP2Regs *regs);

void gteSQR_part_noshift(struct psxCP2Regs *regs);
void gteSQR_part_shift(struct psxCP2Regs *regs);
void gteOP_part_noshift(struct psxCP2Regs *regs);
void gteOP_part_shift(struct psxCP2Regs *regs);
void gteDCPL_part(struct psxCP2Regs *regs);
void gteGPF_part_noshift(struct psxCP2Regs *regs);
void gteGPF_part_shift(struct psxCP2Regs *regs);

void gteGPL_part_noshift(struct psxCP2Regs *regs);
void gteGPL_part_shift(struct psxCP2Regs *regs);
void gteDPCS_part_noshift(struct psxCP2Regs *regs);
void gteDPCS_part_shift(struct psxCP2Regs *regs);
void gteINTPL_part_noshift(struct psxCP2Regs *regs);
void gteINTPL_part_shift(struct psxCP2Regs *regs);
void gteMACtoRGB(struct psxCP2Regs *regs);

#ifdef __cplusplus
}
#endif
#endif
