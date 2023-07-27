/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* scu_dsp_mvi.cpp - SCU DSP MVI Instructions Emulation
**  Copyright (C) 2015-2018 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ss.h"
#include "scu.h"

#include "scu_dsp_common.inc"

template<bool looped, unsigned dest, unsigned cond>
static NO_INLINE NO_CLONE void MVIInstr(void)
{
 const uint32 instr = DSP_InstrPre<looped>();
 uint32 imm;

 if(cond & 0x40)
  imm = sign_x_to_s32(19, instr);
 else
  imm = sign_x_to_s32(25, instr);

 if(DSP_TestCond<cond>())
 {
  if(DSP.PRAMDMABufCount && (dest == 0x6 || dest == 0x7))
  {
   DSP.PC--;
   //
   DSP_FinishPRAMDMA();
  }

  switch(dest)
  {
   default:
	break;

   case 0x0:
   case 0x1:
   case 0x2:
   case 0x3:
	DSP.DataRAM[dest][DSP.CT[dest]] = imm;
	DSP.CT[dest] = (DSP.CT[dest] + 1) & 0x3F;
	break;

   case 0x4: DSP.RX = imm; break;
   case 0x5: DSP.P.T = (int32)imm; break;

   case 0x6: DSP.RAO = imm; break;

   case 0x7: DSP.WAO = imm; break;
 
   case 0xA: if(!looped || DSP.LOP == 0x0FFF) { DSP.LOP = imm & 0x0FFF; } break;

   case 0xC:
	DSP.TOP = DSP.PC - 1;
	DSP.PC = imm & 0xFF;
        //
	if(DSP.PRAMDMABufCount)
	 DSP_FinishPRAMDMA();
	break;
  }
 }
}


MDFN_HIDE extern void (*const DSP_MVIFuncTable[2][16][128])(void) =
{
 #include "scu_dsp_mvitab.inc"
};
