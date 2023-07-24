/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* jpkeyboard.cpp:
**  Copyright (C) 2017-2018 Mednafen Team
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

//
// Compared to the PS/2 keyboard adapter+PS/2 keyboard, the Japanese Saturn keyboard has the following differences:
//   A few extra Japanese language keys, and related keymap/layout differences.
//   No numpad and associated keys.
//   Pause key behaves like a normal key.
//   Caps Lock/Scroll Lock key repeat doesn't cause the lock state to oscillate.
//   Lock status is hidden to the user(no LEDs), but still visible to software(though software may not respect it).
//   Key repeat delay counters are apparently clocked by polling/doing read sequences(instead of having an independent internal clock), but only when a key make/break
//	isn't pending(so releasing keys kind of messes up the repeat rate of a held key a little).
//
// TODO:
//	Emulate limitations on multiple keys being pressed simultaneously.
//

#include "common.h"
#include "jpkeyboard.h"


IODevice_JPKeyboard::IODevice_JPKeyboard() : phys{0,0,0,0}
{

}

IODevice_JPKeyboard::~IODevice_JPKeyboard()
{

}

void IODevice_JPKeyboard::Power(void)
{
 phase = -1;
 tl = true;
 data_out = 0x01;

 simbutt = simbutt_pend = 0;
 lock = lock_pend = 0;

 mkbrk_pend = 0;
 memset(buffer, 0, sizeof(buffer));

 memset(processed, 0, sizeof(processed));
 memset(fifo, 0, sizeof(fifo));
 fifo_rdp = 0;
 fifo_wrp = 0;
 fifo_cnt = 0;

 rep_sc = rep_sc_pend = 0;
 rep_dcnt = rep_dcnt_pend = 0;
}

void IODevice_JPKeyboard::UpdateInput(const uint8* data, const int32 time_elapsed)
{
 phys[0] = MDFN_de64lsb(&data[0x00]);
 phys[1] = MDFN_de64lsb(&data[0x08]);
 phys[2] = MDFN_de16lsb(&data[0x10]);
 phys[3] = 0;
 //

 for(unsigned i = 0; i < 4; i++)
 {
  uint64 tmp = phys[i] ^ processed[i];
  unsigned bp;

  while((bp = (63 ^ MDFN_lzcount64(tmp))) < 64)
  {
   const uint64 mask = ((uint64)1 << bp);
   const int sc = ((i << 6) + bp);

   if(fifo_cnt >= fifo_size)
    goto fifo_oflow_abort;

   if(phys[i] & mask)
   {
    fifo[fifo_wrp] = 0x800 | sc;
    fifo_wrp = (fifo_wrp + 1) % fifo_size;
    fifo_cnt++;
   }

   if(!(phys[i] & mask))
   {
    fifo[fifo_wrp] = 0x100 | sc;
    fifo_wrp = (fifo_wrp + 1) % fifo_size;
    fifo_cnt++;
   }

   processed[i] = (processed[i] & ~mask) | (phys[i] & mask);
   tmp &= ~mask;
  }
 }
 fifo_oflow_abort:;
}

void IODevice_JPKeyboard::StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname_prefix)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(fifo),
  SFVAR(fifo_rdp),
  SFVAR(fifo_wrp),
  SFVAR(fifo_cnt),

  SFVAR(phys),
  SFVAR(processed),

  SFVAR(simbutt),
  SFVAR(simbutt_pend),
  SFVAR(lock),
  SFVAR(lock_pend),

  SFVAR(rep_sc),
  SFVAR(rep_sc_pend),
  SFVAR(rep_dcnt),
  SFVAR(rep_dcnt_pend),

  SFVAR(mkbrk_pend),
  SFVAR(buffer),

  SFVAR(data_out),
  SFVAR(tl),

  SFVAR(phase),
  SFEND
 };
 char section_name[64];
 trio_snprintf(section_name, sizeof(section_name), "%s_Keyboard", sname_prefix);

 if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, section_name, true) && load)
  Power();
 else if(load)
 {
  fifo_rdp %= fifo_size;
  fifo_wrp %= fifo_size;
  fifo_cnt %= fifo_size + 1;
  if(phase < 0)
   phase = -1;
  else
   phase %= 12;
 }
}

uint8 IODevice_JPKeyboard::UpdateBus(const sscpu_timestamp_t timestamp, const uint8 smpc_out, const uint8 smpc_out_asserted)
{
 if(smpc_out & 0x40)
 {
  phase = -1;
  tl = true;
  data_out = 0x01;
 }
 else
 {
  if((bool)(smpc_out & 0x20) != tl)
  {
   if(phase < 11)
   {
    tl = !tl;
    phase++;
   }

   if(!phase)
   {
    if(mkbrk_pend == (uint8)mkbrk_pend)
    {
     if(fifo_cnt)
     {
      mkbrk_pend = fifo[fifo_rdp];
      fifo_rdp = (fifo_rdp + 1) % fifo_size;
      fifo_cnt--;

      //
      bool p = mkbrk_pend & 0x800;

      if(p)
      {
       rep_sc_pend = mkbrk_pend & 0xFF;
       rep_dcnt_pend = 30;
      }
      else
      {
       if(rep_sc == (mkbrk_pend & 0xFF))
        rep_dcnt_pend = 0;
      }

      switch(mkbrk_pend & 0xFF)
      {
       case 0x89: /*  Up */ simbutt_pend = simbutt & ~(1 <<  0); simbutt_pend &= ~(p <<  1); simbutt_pend |= (p <<  0); break;
       case 0x8A: /*Down */ simbutt_pend = simbutt & ~(1 <<  1); simbutt_pend &= ~(p <<  0); simbutt_pend |= (p <<  1); break;
       case 0x86: /*Left */ simbutt_pend = simbutt & ~(1 <<  2); simbutt_pend &= ~(p <<  3); simbutt_pend |= (p <<  2); break;
       case 0x8D: /*Right*/ simbutt_pend = simbutt & ~(1 <<  3); simbutt_pend &= ~(p <<  2); simbutt_pend |= (p <<  3); break;
       case 0x22: /*   X */ simbutt_pend = simbutt & ~(1 <<  4); simbutt_pend |= (p <<  4); break;
       case 0x21: /*   C */ simbutt_pend = simbutt & ~(1 <<  5); simbutt_pend |= (p <<  5); break;
       case 0x1A: /*   Z */ simbutt_pend = simbutt & ~(1 <<  6); simbutt_pend |= (p <<  6); break;
       case 0x76: /* Esc */ simbutt_pend = simbutt & ~(1 <<  7); simbutt_pend |= (p <<  7); break;
       case 0x23: /*   D */ simbutt_pend = simbutt & ~(1 <<  8); simbutt_pend |= (p <<  8); break;
       case 0x1B: /*   S */ simbutt_pend = simbutt & ~(1 <<  9); simbutt_pend |= (p <<  9); break;
       case 0x1C: /*   A */ simbutt_pend = simbutt & ~(1 << 10); simbutt_pend |= (p << 10); break;
       case 0x24: /*   E */ simbutt_pend = simbutt & ~(1 << 11); simbutt_pend |= (p << 11); break;
       case 0x15: /*   Q */ simbutt_pend = simbutt & ~(1 << 15); simbutt_pend |= (p << 15); break;

       case 0x7E: /* Scrl */ lock_pend = lock ^ (p ? LOCK_SCROLL : 0); break;
       case 0x58: /* Caps */ lock_pend = lock ^ (p ? LOCK_CAPS : 0);   break;
      }
     }
     else if(rep_dcnt > 0)
     {
      rep_dcnt_pend = rep_dcnt - 1;

      if(!rep_dcnt_pend)
      {
       mkbrk_pend = 0x800 | rep_sc;
       rep_dcnt_pend = 6;
      }
     }
    }
    buffer[ 0] = 0x3;
    buffer[ 1] = 0x4;
    buffer[ 2] = (((simbutt_pend >>  0) ^ 0xF) & 0xF);
    buffer[ 3] = (((simbutt_pend >>  4) ^ 0xF) & 0xF);
    buffer[ 4] = (((simbutt_pend >>  8) ^ 0xF) & 0xF);
    buffer[ 5] = (((simbutt_pend >> 12) ^ 0xF) & 0x8) | 0x0;
    buffer[ 6] = lock_pend;
    buffer[ 7] = ((mkbrk_pend >> 8) & 0xF) | 0x6;
    buffer[ 8] =  (mkbrk_pend >> 4) & 0xF;
    buffer[ 9] =  (mkbrk_pend >> 0) & 0xF;
    buffer[10] = 0x0;
    buffer[11] = 0x1;
   }

   if(phase == 9)
   {
    mkbrk_pend = (uint8)mkbrk_pend;
    lock = lock_pend;
    simbutt = simbutt_pend;
    rep_dcnt = rep_dcnt_pend;
    rep_sc = rep_sc_pend;
   }

   data_out = buffer[phase];
  }
 }

 return (smpc_out & (smpc_out_asserted | 0xE0)) | (((tl << 4) | data_out) &~ smpc_out_asserted);
}
