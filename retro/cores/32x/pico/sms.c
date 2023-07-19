/*
 * SMS emulation
 * (C) notaz, 2009-2010
 * (C) kub, 2021
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */
/*
 * TODO:
 * - start in a state as if BIOS ran (partly done for VDP registers, RAM)
 * - region support (currently only very limited PAL and Mark-III support)
 * - mapper for EEPROM support
 */
#include "pico_int.h"
#include "memory.h"
#include "sound/sn76496.h"
#include "sound/emu2413/emu2413.h"

extern void YM2413_regWrite(unsigned reg);
extern void YM2413_dataWrite(unsigned data);

extern unsigned sprites_status; // TODO put in some hdr file!

static unsigned char vdp_data_read(void)
{
  struct PicoVideo *pv = &Pico.video;
  unsigned char d;

  d = Pico.ms.vdp_buffer;
  Pico.ms.vdp_buffer = PicoMem.vramb[MEM_LE2(pv->addr)];
  pv->addr = (pv->addr + 1) & 0x3fff;
  pv->pending = 0;
  return d;
}

static unsigned char vdp_ctl_read(void)
{
  struct PicoVideo *pv = &Pico.video;
  unsigned char d;

  z80_int_assert(0);
  d = pv->status | (pv->pending_ints << 7);
  pv->pending = pv->pending_ints = 0;
  pv->status = 0;

  if (pv->reg[0] & 0x04)
    d |= 0x1f; // unused bits in mode 4 read as 1

  elprintf(EL_SR, "VDP sr: %02x", d);
  return d;
}

static void vdp_data_write(unsigned char d)
{
  struct PicoVideo *pv = &Pico.video;

  if (pv->type == 3) {
    // cram. 32 on SMS, but 64 on MD. Fill 2nd half of cram for prio bit mirror
    if (PicoIn.AHW & PAHW_GG) { // GG, same layout as MD
      unsigned a = pv->addr & 0x3f;
      if (a & 0x1) { // write complete color on high byte write
        u16 c = ((d&0x0f) << 8) | Pico.ms.vdp_buffer;
        if (PicoMem.cram[a >> 1] != c) Pico.m.dirtyPal = 1;
        PicoMem.cram[a >> 1] = PicoMem.cram[(a >> 1)+0x20] = c;
      }
    } else { // SMS, convert to MD layout (00BbGgRr to 0000BbBbGgGgRrRr)
      unsigned a = pv->addr & 0x1f;
      u16 c = ((d&0x30)<<6) + ((d&0x0c)<<4) + ((d&0x03)<<2);
      if (PicoMem.cram[a] != (c | (c>>2))) Pico.m.dirtyPal = 1;
      PicoMem.cram[a] = PicoMem.cram[a+0x20] = c | (c>>2);
    }
  } else {
    PicoMem.vramb[MEM_LE2(pv->addr)] = d;
  }
  pv->addr = (pv->addr + 1) & 0x3fff;

  Pico.ms.vdp_buffer = d;
  pv->pending = 0;
}

static NOINLINE void vdp_reg_write(struct PicoVideo *pv, u8 a, u8 d)
{
  int l;

  pv->reg[a] = d;
  switch (a) {
  case 0:
    l = pv->pending_ints & (d >> 3) & 2;
    elprintf(EL_INTS, "hint %d", l);
    z80_int_assert(l);
    break;
  case 1:
    l = pv->pending_ints & (d >> 5) & 1;
    elprintf(EL_INTS, "vint %d", l);
    z80_int_assert(l);
    break;
  }
}

static void vdp_ctl_write(u8 d)
{
  struct PicoVideo *pv = &Pico.video;

  if (pv->pending) {
    pv->type = d >> 6;
    if (pv->type == 2) {
      elprintf(EL_IO, "  VDP r%02x=%02x", d & 0x0f, pv->addr & 0xff);
      if (pv->reg[d & 0x0f] != (u8)pv->addr)
        vdp_reg_write(pv, d & 0x0f, pv->addr);
    }
    pv->addr &= 0x00ff;
    pv->addr |= (d & 0x3f) << 8;
    if (pv->type == 0) {
      Pico.ms.vdp_buffer = PicoMem.vramb[MEM_LE2(pv->addr)];
      pv->addr = (pv->addr + 1) & 0x3fff;
    }
  } else {
    pv->addr &= 0x3f00;
    pv->addr |= d;
  }
  pv->pending ^= 1;
}

static u8 vdp_hcounter(int cycles)
{
  // 171 slots per scanline of 228 clocks, counted 0xf4-0x93, 0xe9-0xf3
  // this matches h counter tables in SMSVDPTest:
  //  hc =   (cycles+2) *   171    /228      -1 + 0xf4;
  int hc = (((cycles+2) * ((171<<8)/228))>>8)-1 + 0xf4; // Q8 to avoid dividing
  if (hc > 0x193) hc += 0xe9-0x93-1;
  return hc;
}

static unsigned char z80_sms_in(unsigned short a)
{
  unsigned char d = 0xff;

  a &= 0xff;
  elprintf(EL_IO, "z80 port %04x read", a);
  if(a >= 0xf0){
    if (PicoIn.opt & POPT_EN_YM2413){
      switch(a)
      {
      case 0xf0:
        // FM reg port
        break;
      case 0xf1:
        // FM data port
        break;
      case 0xf2:
        // bit 0 = 1 active FM Pac
        d = 0xf8 | Pico.ms.fm_ctl;
        break;
      }
    }
  }
  else{
    switch (a & 0xc1)
    {
      case 0x00:
      case 0x01:
        if ((PicoIn.AHW & PAHW_GG) && a < 0x8) { // GG I/O area
          switch (a) {
          case 0: d = 0xff & ~(PicoIn.pad[0] & 0x80);               break;
          case 1: d = Pico.ms.io_gg[1] | (Pico.ms.io_gg[2] & 0x7f); break;
          case 5: d = Pico.ms.io_gg[5] & 0xf8;                      break;
          default: d = Pico.ms.io_gg[a];                            break;
          }
        }
        break;

      case 0x40: /* V counter */
        d = Pico.video.v_counter;
        elprintf(EL_HVCNT, "V counter read: %02x", d);
        break;

      case 0x41: /* H counter */
        d = Pico.ms.vdp_hlatch;
        elprintf(EL_HVCNT, "H counter read: %02x", d);
        break;

      case 0x80:
        d = vdp_data_read();
        break;

      case 0x81:
        d = vdp_ctl_read();
        break;

      case 0xc0: /* I/O port A and B */
        if (! (PicoIn.AHW & PAHW_SC) || (Pico.ms.io_sg & 7) == 7)
          d = ~((PicoIn.pad[0] & 0x3f) | (PicoIn.pad[1] << 6));
        else
          ; // read kbd 8 bits
        break;

      case 0xc1: /* I/O port B and miscellaneous */
        if (! (PicoIn.AHW & PAHW_SC) || (Pico.ms.io_sg & 7) == 7) {
          d = (Pico.ms.io_ctl & 0x80) | ((Pico.ms.io_ctl << 1) & 0x40) | 0x30;
          d |= ~(PicoIn.pad[1] >> 2) & 0x0f;
          if (Pico.ms.io_ctl & 0x08) d |= 0x80; // TH as input is unconnected
          if (Pico.ms.io_ctl & 0x02) d |= 0x40;
        } else
          ; // read kbd 4 bits
        break;
    }
  }
  elprintf(EL_IO, "ret = %02x", d);
  return d;
}

static void z80_sms_out(unsigned short a, unsigned char d)
{
  elprintf(EL_IO, "z80 port %04x write %02x", a, d);

  a &= 0xff;
  if (a >= 0xf0){
    if (PicoIn.opt & POPT_EN_YM2413){
      switch(a)
      {
        case 0xf0:
          // FM reg port
          YM2413_regWrite(d);
          break;
        case 0xf1:
          // FM data port
          YM2413_dataWrite(d);
          break;
        case 0xf2:
          // bit 0 = 1 active FM Pac
          Pico.ms.fm_ctl = d & 0x1;
          break;
      }
    }
  }
  else {
    switch (a & 0xc1)
    {
      case 0x00:
        if ((PicoIn.AHW & PAHW_GG) && a < 0x8)   // GG I/O area
          Pico.ms.io_gg[a] = d;
        break;
      case 0x01:
        if ((PicoIn.AHW & PAHW_GG) && a < 0x8) { // GG I/O area
          Pico.ms.io_gg[a] = d;
        } else {
          // pad. latch hcounter if one of the TH lines is switched to 1
          if ((Pico.ms.io_ctl ^ d) & d & 0xa0)
            Pico.ms.vdp_hlatch = vdp_hcounter(z80_cyclesDone() - Pico.t.z80c_line_start);
          Pico.ms.io_ctl = d;
        }
        break;

      case 0x40:
      case 0x41:
        PsndDoPSG(z80_cyclesDone());
        SN76496Write(d);
        break;

      case 0x80:
        vdp_data_write(d);
        break;

      case 0x81:
        vdp_ctl_write(d);
        break;

      case 0xc0:
        if ((PicoIn.AHW & PAHW_SC) && (a & 0x2))
          Pico.ms.io_sg = d; // 0xc2 = kbd/pad select
    }
  }
}

static void z80_exec(int aim)
{
  Pico.t.z80c_aim = aim;
  Pico.t.z80c_cnt += z80_run(Pico.t.z80c_aim - Pico.t.z80c_cnt);
}


// ROM/SRAM bank mapping, see https://www.smspower.org/Development/Mappers

static int bank_mask;

static void xwrite(unsigned int a, unsigned char d);


// Sega mapper. Maps 3 banks 16KB each, with SRAM support
static void write_sram_sega(unsigned short a, unsigned char d)
{
  // SRAM is mapped in 2 16KB banks, selected by bit 2 in control reg
  a &= 0x3fff;
  a += ((Pico.ms.carthw[0x0c] & 0x04) >> 2) * 0x4000;

  Pico.sv.changed |= (Pico.sv.data[a] != d);
  Pico.sv.data[a] = d;
}

static void write_bank_sega(unsigned short a, unsigned char d)
{
  if (a < 0xfff8) return;
  // avoid mapper detection for RAM fill with 0
  if (Pico.ms.mapper != PMS_MAP_SEGA && (Pico.ms.mapper || d == 0)) return;

  elprintf(EL_Z80BNK, "bank sega %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_SEGA;
  Pico.ms.carthw[a & 0x0f] = d;

  switch (a & 0x0f)
  {
    case 0x0d:
      d &= bank_mask;
      z80_map_set(z80_read_map, 0x0400, 0x3fff, Pico.rom+0x400 + (d << 14), 0);
      break;
    case 0x0e:
      d &= bank_mask;
      z80_map_set(z80_read_map, 0x4000, 0x7fff, Pico.rom + (d << 14), 0);
      break;

    case 0x0c:
      if (d & ~0x8c)
        elprintf(EL_STATUS|EL_ANOMALY, "%02x written to control reg!", d);
      /*FALLTHROUGH*/
    case 0x0f:
      if (Pico.ms.carthw[0xc] & 0x08) {
        d = (Pico.ms.carthw[0xc] & 0x04) >> 2;
        z80_map_set(z80_read_map, 0x8000, 0xbfff, Pico.sv.data + d*0x4000, 0);
        z80_map_set(z80_write_map, 0x8000, 0xbfff, write_sram_sega, 1);
      } else {
        d = Pico.ms.carthw[0xf] & bank_mask;
        z80_map_set(z80_read_map, 0x8000, 0xbfff, Pico.rom + (d << 14), 0);
        z80_map_set(z80_write_map, 0x8000, 0xbfff, xwrite, 1);
      }
      break;
  }
}

// Codemasters mapper. Similar to Sega, but different addresses, TODO: SRAM
static void write_bank_codem(unsigned short a, unsigned char d)
{
  if (a >= 0xc000 || (a & 0x3fff)) return; // address is 0x0000, 0x4000, 0x8000?
  // don't detect linear mapping to avoid confusing with MSX
  if (Pico.ms.mapper != PMS_MAP_CODEM && (Pico.ms.mapper || (a>>14) == d)) return;
  elprintf(EL_Z80BNK, "bank codem %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_CODEM;
  Pico.ms.carthw[a>>14] = d;

  d &= bank_mask;
  z80_map_set(z80_read_map, a, a+0x3fff, Pico.rom + (d << 14), 0);
}

// MSX mapper. 4 selectable 8KB banks at the top
static void write_bank_msx(unsigned short a, unsigned char d)
{
  if (a > 0x0003 || !(Pico.m.hardware & PMS_HW_JAP)) return;
  // don't detect linear mapping to avoid confusing with Codemasters
  if (Pico.ms.mapper != PMS_MAP_MSX && (Pico.ms.mapper || (a|d) == 0 || d >= 0x80)) return;
  elprintf(EL_Z80BNK, "bank msx %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_MSX;
  Pico.ms.carthw[a] = d;

  a = (a^2)*0x2000 + 0x4000;
  d &= 2*bank_mask + 1;
  z80_map_set(z80_read_map, a, a+0x1fff, Pico.rom + (d << 13), 0);
}

// Korea mapping, 1 selectable 16KB bank at the top
static void write_bank_korea(unsigned short a, unsigned char d)
{
  if (a != 0xa000) return;
  if (Pico.ms.mapper != PMS_MAP_KOREA && (Pico.ms.mapper)) return;
  elprintf(EL_Z80BNK, "bank korea %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_KOREA;
  Pico.ms.carthw[0xf] = d;

  d &= bank_mask;
  z80_map_set(z80_read_map, 0x8000, 0xbfff, Pico.rom + (d << 14), 0);
}

// Korean n-in-1 mapping. 1 selectable 32KB bank at the bottom
static void write_bank_n32k(unsigned short a, unsigned char d)
{
  if (a != 0xffff) return;
  // code must be in RAM since all visible ROM space is swapped
  if (Pico.ms.mapper != PMS_MAP_N32K && (Pico.ms.mapper || z80_pc() < 0xc000)) return;
  elprintf(EL_Z80BNK, "bank 32k %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_N32K;
  Pico.ms.carthw[0xf] = d;

  d &= bank_mask >> 1;
  z80_map_set(z80_read_map, 0,   0x7fff, Pico.rom + (d << 15), 0);
}

// Korean 4-in-1. 2 selectable 16KB banks, top bank is shifted by bottom one
static void write_bank_n16k(unsigned short a, unsigned char d)
{
  if (a != 0x3ffe && a != 0x7fff && a != 0xbfff) return;
  // code must be in RAM since all visible ROM space is swapped
  if (Pico.ms.mapper != PMS_MAP_N16K && (Pico.ms.mapper || z80_pc() < 0xc000)) return;
  elprintf(EL_Z80BNK, "bank 16k %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_N16K;
  Pico.ms.carthw[a>>14] = d;

  d &= bank_mask;
  a = a & 0xc000;
  // the top bank shifts with the bottom bank.
  if (a == 0x8000) d += Pico.ms.carthw[0] & 0x30;
  z80_map_set(z80_read_map, a, a+0x3fff, Pico.rom + (d << 14), 0);
}

// MSX-Nemesis mapper. 4 selectable 8KB banks at the top
static void write_bank_msxn(unsigned short a, unsigned char d)
{
  if (a > 0x0003) return;
  // never autodetected, selectable only via config
  if (Pico.ms.mapper != PMS_MAP_NEMESIS) return;
  elprintf(EL_Z80BNK, "bank nems %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.carthw[a] = d;

  a = (a^2)*0x2000 + 0x4000;
  d &= 2*bank_mask + 1;
  z80_map_set(z80_read_map, a, a+0x1fff, Pico.rom + (d << 13), 0);
}

// Korean Janggun mapper. 4 selectable 8KB banks at the top, hardware byte flip
static unsigned char read_flipped_jang(unsigned a)
{
  static unsigned char flipper[16] = // reversed nibble bit order
      { 0x0,0x8,0x4,0xc,0x2,0xa,0x6,0xe,0x1,0x9,0x5,0xd,0x3,0xb,0x7,0xf };
  unsigned char c;

  // return value at address a in reversed bit order
  c = Pico.rom[(Pico.ms.carthw[a>>13] << 13) + (a & 0x1fff)];
  return (flipper[c&0xf]<<4) | flipper[c>>4];
}

static void write_bank_jang(unsigned short a, unsigned char d)
{
  // address is 0xfffe, 0xffff, 0x4000, 0x6000, 0x8000, 0xa000
  if ((a|1) != 0xffff && (!((a^(a<<1)) & 0x8000) || (a & 0x1fff))) return;
  // never autodetected, selectable only via config
  if (Pico.ms.mapper != PMS_MAP_JANGGUN) return;
  elprintf(EL_Z80BNK, "bank jang %04x %02x @ %04x", a, d, z80_pc());

  if ((a|1) == 0xffff) {
    int x = a & 1, f = d & 0x40;
    Pico.ms.carthw[x] = d;
    d &= bank_mask;
    Pico.ms.carthw[2*x + 2] = 2*d, Pico.ms.carthw[2*x + 3] = 2*d+1;
    a = (x+1) * 0x4000;
    if (!f)
      z80_map_set(z80_read_map, a, a+0x3fff, Pico.rom + (d << 14), 0);
    else
      z80_map_set(z80_read_map, a, a+0x3fff, read_flipped_jang, 1);
  } else {
    d &= 2*bank_mask + 1;
    Pico.ms.carthw[a>>13] = d;
    if (!(Pico.ms.carthw[(a>>15)&1] & 0x40))
      z80_map_set(z80_read_map, a, a+0x1fff, Pico.rom + (d << 13), 0);
    else
      z80_map_set(z80_read_map, a, a+0x1fff, read_flipped_jang, 1);
  }
}

// Korean 188-in-1. 4 8KB banks from 0x4000, selected by xor'd bank index
static void write_bank_xor(unsigned short a, unsigned char d)
{
  // 4x8KB bank select @0x2000
  if ((a&0x6000) != 0x2000) return;
  if (Pico.ms.mapper != PMS_MAP_XOR && Pico.ms.mapper) return;

  elprintf(EL_Z80BNK, "bank xor %04x %02x @ %04x", a, d, z80_pc());
  Pico.ms.mapper = PMS_MAP_XOR;

  Pico.ms.carthw[0] = d;
  z80_map_set(z80_read_map,  0x4000, 0x5fff, Pico.rom + ((d^0x1f) << 13), 0);
  z80_map_set(z80_read_map,  0x6000, 0x7fff, Pico.rom + ((d^0x1e) << 13), 0);
  z80_map_set(z80_read_map,  0x8000, 0x9fff, Pico.rom + ((d^0x1d) << 13), 0);
  z80_map_set(z80_read_map,  0xa000, 0xbfff, Pico.rom + ((d^0x1c) << 13), 0);
}

// SG-1000 8KB RAM Adaptor mapper. 8KB RAM at address 0x2000
static void write_bank_x8k(unsigned short a, unsigned char d)
{
  // 8KB address range @ 0x2000 (adaptor) or @ 0x8000 (cartridge)
  if ((a&0xe000) != 0x2000 && (a&0xe000) != 0x8000) return;
  if (Pico.ms.mapper != PMS_MAP_8KBRAM && Pico.ms.mapper) return;

  elprintf(EL_Z80BNK, "bank x8k %04x %02x @ %04x", a, d, z80_pc());
  ((unsigned char *)(PicoMem.vram+0x4000))[a&0x1fff] = d;
  Pico.ms.mapper = PMS_MAP_8KBRAM;

  a &= 0xe000;
  Pico.ms.carthw[0] = a >> 12;
  z80_map_set(z80_read_map,  a, a+0x1fff, PicoMem.vram+0x4000, 0);
  z80_map_set(z80_write_map, a, a+0x1fff, PicoMem.vram+0x4000, 0);
}

// SC-3000 32KB RAM mapper for BASIC level IIIB. 32KB RAM at address 0x8000
static void write_bank_x32k(unsigned short a, unsigned char d)
{
  // 32KB address range @ 0x8000
  if ((a&0xc000) != 0x8000) return;
  if (Pico.ms.mapper != PMS_MAP_32KBRAM &&
      (Pico.ms.mapper || Pico.romsize > 0x8000)) return;

  elprintf(EL_Z80BNK, "bank x32k %04x %02x @ %04x", a, d, z80_pc());
  ((unsigned char *)(PicoMem.vram+0x4000))[a&0x7fff] = d;
  Pico.ms.mapper = PMS_MAP_32KBRAM;

  a &= 0xc000;
  Pico.ms.carthw[0] = a >> 12;
  // NB this deactivates internal RAM and all mapper detection
  z80_map_set(z80_read_map,  a, a+0x7fff, PicoMem.vram+0x4000, 0);
  z80_map_set(z80_write_map, a, a+0x7fff, PicoMem.vram+0x4000, 0);
}

char *mappers[] = {
  [PMS_MAP_SEGA]     = "Sega",
  [PMS_MAP_CODEM]    = "Codemasters",
  [PMS_MAP_KOREA]    = "Korea",
  [PMS_MAP_MSX]      = "Korea MSX",
  [PMS_MAP_N32K]     = "Korea X-in-1",
  [PMS_MAP_N16K]     = "Korea 4-Pak",
  [PMS_MAP_JANGGUN]  = "Korea Janggun",
  [PMS_MAP_NEMESIS]  = "Korea Nemesis",
  [PMS_MAP_8KBRAM]   = "Taiwan 8K RAM",
  [PMS_MAP_XOR]      = "Korea XOR",
  [PMS_MAP_32KBRAM]  = "Sega 32K RAM",
};

// TODO auto-selecting is not really reliable.
// Before adding more mappers this should be revised.
static void xwrite(unsigned int a, unsigned char d)
{
  int sz = (PicoIn.AHW & (PAHW_SG|PAHW_SC) ? 2 : 8) * 1024;

  elprintf(EL_IO, "z80 write [%04x] %02x", a, d);
  if (a >= 0xc000)
    PicoMem.zram[a & (sz-1)] = d;

  switch (Pico.ms.mapper) { // via config, or auto detected
  case PMS_MAP_SEGA:    write_bank_sega(a, d);  break;
  case PMS_MAP_CODEM:	write_bank_codem(a, d); break;
  case PMS_MAP_MSX:	write_bank_msx(a, d);   break;
  case PMS_MAP_KOREA:	write_bank_korea(a, d); break;
  case PMS_MAP_N32K:	write_bank_n32k(a, d);  break;
  case PMS_MAP_N16K:	write_bank_n16k(a, d);  break;
  case PMS_MAP_JANGGUN: write_bank_jang(a, d);  break;
  case PMS_MAP_NEMESIS: write_bank_msxn(a, d);  break;
  case PMS_MAP_8KBRAM:  write_bank_x8k(a, d);   break;
  case PMS_MAP_32KBRAM: write_bank_x32k(a, d);  break;
  case PMS_MAP_XOR:     write_bank_xor(a, d);   break;

  case PMS_MAP_AUTO:
        // disable autodetection after some time
        if ((a >= 0xc000 && a < 0xfff8) || Pico.ms.mapcnt > 20) break;
        // NB the sequence of mappers is crucial for the auto detection
        if (PicoIn.AHW & PAHW_SC) {
          write_bank_x32k(a,d);
        } else if (PicoIn.AHW & PAHW_SG) {
          write_bank_x8k(a, d);
        } else {
          write_bank_n32k(a, d);
          write_bank_sega(a, d);
          write_bank_msx(a, d);
          write_bank_codem(a, d);
          write_bank_korea(a, d);
          write_bank_n16k(a, d);
          write_bank_xor(a, d);
        }

        Pico.ms.mapcnt ++;
        if (Pico.ms.mapper)
          elprintf(EL_STATUS, "autodetected %s mapper",mappers[Pico.ms.mapper]);
        break;
  }
}

// TMR product codes and hardware type for known 50Hz-only games
static u32 region_pal[] = { // cf. GX+, core/cart_hw/sms_cartc.c
  0x40207067 /* Addams Family */, 0x40207020 /* Back.Future 3 */,
  0x40207058 /* Battlemaniacs */, 0x40007105 /* Cal.Games 2 */,
  0x40207065 /* Dracula */      , 0x40007109 /* Home Alone */,
  0x40009024 /* Pwr.Strike 2 */ , 0x40207047 /* Predator 2 EU */,
  0x40002519 /* Quest.Yak */    , 0x40207064 /* Robocop 3 */,
  0x40205014 /* Sens.Soccer */  , 0x40002573 /* Sonic Blast */,
  0x40007080 /* S.Harrier EU */ , 0x40007038 /* Taito Chase */,
};

void PicoResetMS(void)
{
  unsigned tmr;
  int id, hw, i;

  // set preselected hw/mapper from config
  if (PicoIn.hwSelect) {
    PicoIn.AHW &= ~(PAHW_GG|PAHW_SG|PAHW_SC);
    switch (PicoIn.hwSelect) {
    case PHWS_GG:  PicoIn.AHW |= PAHW_GG; break;
    case PHWS_SG:  PicoIn.AHW |= PAHW_SG; break;
    case PHWS_SC:  PicoIn.AHW |= PAHW_SC; break;
    }
  }
  Pico.ms.mapcnt = Pico.ms.mapper = 0;
  if (PicoIn.mapper)
    Pico.ms.mapper = PicoIn.mapper;
  Pico.m.hardware |= PMS_HW_JAP; // default region Japan if no TMR header
  if (PicoIn.regionOverride > 2)
    Pico.m.hardware &= ~PMS_HW_JAP;

  // check if the ROM header contains more system information
  for (tmr = 0x2000; tmr < 0xbfff && tmr <= Pico.romsize; tmr *= 2) {
    if (!memcmp(Pico.rom + tmr-16, "TMR SEGA", 8)) {
      hw = Pico.rom[tmr-1] >> 4;
      if (!PicoIn.hwSelect) {
        PicoIn.AHW &= ~(PAHW_GG|PAHW_SG|PAHW_SC);
        if (hw >= 0x5 && hw < 0x8)
          PicoIn.AHW |= PAHW_GG; // GG cartridge detected
      }
      if (!PicoIn.regionOverride) {
        Pico.m.hardware &= ~PMS_HW_JAP;
        if (hw == 0x5 || hw == 0x3)
          Pico.m.hardware |= PMS_HW_JAP; // region Japan
      }
      id = CPU_LE4(*(u32 *)&Pico.rom[tmr-4]) & 0xf0f0ffff;
      for (i = 0; i < sizeof(region_pal)/sizeof(*region_pal); i++)
        if (id == region_pal[i] && !PicoIn.regionOverride) {
          Pico.m.pal = 1; // requires 50Hz timing
          break;
        }
      break;
    }
  }

  z80_reset();
  PsndReset(); // pal must be known here
  Pico.ms.fm_ctl = 0xff;
  Pico.m.dirtyPal = 1;

  // reset memory mapping
  PicoMemSetupMS();

  // BIOS, VDP intialisation
  Pico.video.reg[0] = 0x36;
  Pico.video.reg[1] = 0xa0;
  Pico.video.reg[2] = 0xff;
  Pico.video.reg[3] = 0xff;
  Pico.video.reg[4] = 0xff;
  Pico.video.reg[5] = 0xff;
  Pico.video.reg[6] = 0xfb;
  Pico.video.reg[7] = 0x00;
  Pico.video.reg[8] = 0x00;
  Pico.video.reg[9] = 0x00;
  Pico.video.reg[10] = 0xff;

  // BIOS, clear zram (unitialized on Mark-III, cf src/mame/drivers/sms.cpp)
  i = !(PicoIn.AHW & PAHW_GG) && (Pico.m.hardware & PMS_HW_JAP) ? 0xf0 : 0x00;
  memset(PicoMem.zram, i, sizeof(PicoMem.zram));
}

void PicoPowerMS(void)
{
  int s, tmp;

  memset(&PicoMem,0,sizeof(PicoMem));
  memset(&Pico.video,0,sizeof(Pico.video));
  memset(&Pico.m,0,sizeof(Pico.m));

  // calculate a mask for bank writes.
  // ROM loader has aligned the size for us, so this is safe.
  s = 0; tmp = Pico.romsize;
  while ((tmp >>= 1) != 0)
    s++;
  if (Pico.romsize > (1 << s))
    s++;
  tmp = 1 << s;
  bank_mask = (tmp - 1) >> 14;

  PicoMem.ioports[0] = 0xc3; // hack to jump @0 at end of RAM to wrap around
  Pico.ms.mapper = PicoIn.mapper;
  PicoReset();
}

void PicoMemSetupMS(void)
{
  u8 mapper = Pico.ms.mapper;
  int sz = (PicoIn.AHW & (PAHW_SG|PAHW_SC) ? 2 : 8) * 1024;
  u32 a;

  // RAM and its mirrors
  for (a = 0xc000; a < 0x10000; a += sz) {
    z80_map_set(z80_read_map, a, a + sz-1, PicoMem.zram, 0);
    z80_map_set(z80_write_map, a, a + sz-1, PicoMem.zram, 0);
  }
  a = 0x10000 - (1<<Z80_MEM_SHIFT);
  z80_map_set(z80_write_map, a, 0xffff, xwrite, 1); // mapper detection

  // ROM
  z80_map_set(z80_read_map, 0x0000, 0xbfff, Pico.rom, 0);
  z80_map_set(z80_write_map, 0x0000, 0xbfff, xwrite, 1); // mapper detection

  // Nemesis mapper maps last 8KB rom bank #15 to adress 0
  if (mapper == PMS_MAP_NEMESIS && Pico.romsize > 0x1e000)
    z80_map_set(z80_read_map, 0x0000, 0x1fff, Pico.rom + 0x1e000, 0);
#ifdef _USE_DRZ80
  drZ80.z80_in = z80_sms_in;
  drZ80.z80_out = z80_sms_out;
#endif
#ifdef _USE_CZ80
  Cz80_Set_INPort(&CZ80, z80_sms_in);
  Cz80_Set_OUTPort(&CZ80, z80_sms_out);
#endif

  // memory mapper setup, linear mapping of 1st 48KB
  memset(Pico.ms.carthw, 0, sizeof(Pico.ms.carthw));
  if (mapper == PMS_MAP_MSX || mapper == PMS_MAP_NEMESIS) {
    xwrite(0x0000, 4);
    xwrite(0x0001, 5);
    xwrite(0x0002, 2);
    xwrite(0x0003, 3);
  } else if (mapper == PMS_MAP_KOREA) {
    xwrite(0xa000, 2);
  } else if (mapper == PMS_MAP_N32K) {
    xwrite(0xffff, 0);
  } else if (mapper == PMS_MAP_N16K) {
    xwrite(0x3ffe, 0);
    xwrite(0x7fff, 1);
    xwrite(0xbfff, 2);
  } else if (mapper == PMS_MAP_JANGGUN) {
    xwrite(0xfffe, 1);
    xwrite(0xffff, 2);
  } else if (mapper == PMS_MAP_XOR) {
    xwrite(0x2000, 0);
  } else if (mapper == PMS_MAP_CODEM) {
    xwrite(0x0000, 0);
    xwrite(0x4000, 1);
    xwrite(0x8000, 2);
  } else if (mapper == PMS_MAP_SEGA) {
    xwrite(0xfffc, 0);
    xwrite(0xfffd, 0);
    xwrite(0xfffe, 1);
    xwrite(0xffff, 2);
  } else if (mapper == PMS_MAP_AUTO) {
    // pre-initialize Sega mapper to linear mapping (else state load may fail)
    Pico.ms.carthw[0xe] = 0x1;
    Pico.ms.carthw[0xf] = 0x2;
  }
}

void PicoStateLoadedMS(void)
{
  u8 mapper = Pico.ms.mapper;
  if (mapper == PMS_MAP_8KBRAM || mapper == PMS_MAP_32KBRAM) {
    u16 a = Pico.ms.carthw[0] << 12;
    xwrite(a, *(unsigned char *)(PicoMem.vram+0x4000));
  } else if (mapper == PMS_MAP_MSX || mapper == PMS_MAP_NEMESIS) {
    xwrite(0x0000, Pico.ms.carthw[0]);
    xwrite(0x0001, Pico.ms.carthw[1]);
    xwrite(0x0002, Pico.ms.carthw[2]);
    xwrite(0x0003, Pico.ms.carthw[3]);
  } else if (mapper == PMS_MAP_KOREA) {
    xwrite(0xa000, Pico.ms.carthw[0x0f]);
  } else if (mapper == PMS_MAP_N32K) {
    xwrite(0xffff, Pico.ms.carthw[0x0f]);
  } else if (mapper == PMS_MAP_N16K) {
    xwrite(0x3ffe, Pico.ms.carthw[0]);
    xwrite(0x7fff, Pico.ms.carthw[1]);
    xwrite(0xbfff, Pico.ms.carthw[2]);
  } else if (mapper == PMS_MAP_JANGGUN) {
    xwrite(0x4000, Pico.ms.carthw[2]);
    xwrite(0x6000, Pico.ms.carthw[3]);
    xwrite(0x8000, Pico.ms.carthw[4]);
    xwrite(0xa000, Pico.ms.carthw[5]);
  } else if (mapper == PMS_MAP_XOR) {
    xwrite(0x2000, Pico.ms.carthw[0]);
  } else if (mapper == PMS_MAP_CODEM) {
    xwrite(0x0000, Pico.ms.carthw[0]);
    xwrite(0x4000, Pico.ms.carthw[1]);
    xwrite(0x8000, Pico.ms.carthw[2]);
  } else if (mapper == PMS_MAP_SEGA) {
    xwrite(0xfffc, Pico.ms.carthw[0x0c]);
    xwrite(0xfffd, Pico.ms.carthw[0x0d]);
    xwrite(0xfffe, Pico.ms.carthw[0x0e]);
    xwrite(0xffff, Pico.ms.carthw[0x0f]);
  }
}

void PicoFrameMS(void)
{
  struct PicoVideo *pv = &Pico.video;
  int is_pal = Pico.m.pal;
  int lines = is_pal ? 313 : 262;
  int cycles_line = 228;
  int skip = PicoIn.skipFrame;
  int lines_vis = 192;
  int hint; // Hint counter
  int nmi;
  int y;

  z80_resetCycles();
  PsndStartFrame();

  // for SMS the pause button generates an NMI, for GG ths is not the case
  nmi = (PicoIn.pad[0] >> 7) & 1;
  if (!(PicoIn.AHW & PAHW_GG) && !Pico.ms.nmi_state && nmi)
    z80_nmi();
  Pico.ms.nmi_state = nmi;

  if ((pv->reg[0] & 6) == 6 && (pv->reg[1] & 0x18))
    lines_vis = (pv->reg[1] & 0x08) ? 240 : 224;
  PicoFrameStartSMS();
  hint = pv->reg[0x0a];

  // SMS: xscroll:f3 sprovr,vint,   vcount:fc, hint:fd
  // GG:  xscroll:f5 sprovr,vint:fd vcount:fe, hint:ff
  for (y = 0; y < lines; y++)
  {
    Pico.t.z80c_line_start = Pico.t.z80c_aim;

    // advance the line counter. It is set back at some point in the VBLANK so
    // that the line count in the active area (-32..lines+1) is contiguous.
    pv->v_counter = Pico.m.scanline = (u8)y;
    switch (is_pal ? -lines_vis : lines_vis) {
    case  192: if (y > 218) pv->v_counter = y - (lines-256); break;
    case  224: if (y > 234) pv->v_counter = y - (lines-256); break;
    case -192: if (y > 242) pv->v_counter = y - (lines-256); break;
    case -224: if (y > 258) pv->v_counter = y - (lines-256); break;
    case -240: if (y > 266) pv->v_counter = y - (lines-256); break;
    }

    // Parse sprites for the next line
    if (y < lines_vis)
      PicoParseSATSMS(y-1);
    else if (y > lines-32)
      PicoParseSATSMS(y-1-lines);

    // render next line
    if (y < lines_vis && !skip)
      PicoLineSMS(y);

    // take over status bits from previously rendered line TODO: cycle exact?
    pv->status |= sprites_status;
    sprites_status = 0;

    // Interrupt handling. Simulate interrupt flagged and immediately reset in
    // same insn by flagging the irq, execute for 1 insn, then checking if the
    // irq is still pending. (GG Chicago, SMS Back to the Future III)
    pv->pending_ints &= ~2; // lost if not caught in the same line
    if (y <= lines_vis)
    {
      if (--hint < 0)
      {
        hint = pv->reg[0x0a];
        pv->pending_ints |= 2;
        z80_exec(Pico.t.z80c_cnt + 1);

        if ((pv->reg[0] & 0x10) && (pv->pending_ints & 2)) {
          elprintf(EL_INTS, "hint");
          z80_int_assert(1);
        }
      }
    }
    else if (y == lines_vis + 1) {
      pv->pending_ints |= 1;
      z80_exec(Pico.t.z80c_cnt + 1);

      if ((pv->reg[1] & 0x20) && (pv->pending_ints & 1)) {
        elprintf(EL_INTS, "vint");
        z80_int_assert(1);
      }
    }

    z80_exec(Pico.t.z80c_line_start + cycles_line);
  }

  PsndGetSamplesMS(lines);
}

void PicoFrameDrawOnlyMS(void)
{
  struct PicoVideo *pv = &Pico.video;
  int lines_vis = 192;
  int y;

  if ((pv->reg[0] & 6) == 6 && (pv->reg[1] & 0x18))
    lines_vis = (pv->reg[1] & 0x08) ? 240 : 224;
  PicoFrameStartSMS();

  for (y = 0; y < lines_vis; y++) {
    PicoParseSATSMS(y-1);
    PicoLineSMS(y);
  }
}

// vim:ts=2:sw=2:expandtab
