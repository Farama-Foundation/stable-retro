/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* scu_dsp_jmp.cpp - SCU DSP JMP Instructions Emulation
**  Copyright (C) 2015-2016 Mednafen Team
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

#pragma GCC optimize("Os")

#include "scu_dsp_common.inc"

template<bool looped, unsigned cond>
static NO_INLINE NO_CLONE void JMPInstr(void)
{
 const uint32 instr = DSP_InstrPre<looped>();

 if(DSP_TestCond<cond>())
  DSP.PC = (uint8)instr;
}

MDFN_HIDE extern void (*const DSP_JMPFuncTable[2][128])(void) =
{
 #include "scu_dsp_jmptab.inc"
};

