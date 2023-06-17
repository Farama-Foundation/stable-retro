/*
 * Sega System 24
 *
 * Kudos to Charles MacDonald (http://cgfm2.emuviews.com) for his
 * very useful research
 *
 */

/* Missing:
   - linescroll in special modes (qgh title, mahmajn2/qrouka attract mode)
   - screen flipping (mix register 13 & 2)
*/

/*
PCB Layout
----------

837-6442 SYSTEM 24 (C) SEGA 1987
|-------------------------------------------------------------------------------|
| YM2151 DSW1 EPR12186.IC1           --------------------------| TMM41464-10 x4 |
|          DSW2         EPR12187.IC2 |--------------------------                |
|                                                CN2                            |
|                 68000                                          TMM41464-10 x4 |
|                |--------------|      315-5295                                 |
|    315-5296    |HITACHI FD1094|      (QFP100)                                 |
|-|  (QFP100)    |317-0058-03D  |                                               |
  |              |--------------|                                               |
|-|                                                              MB81C466-10 x4 |
|                                                                               |
|                                                                               |
|S                                     315-5295                                 |
|E                 20MHz   315-5195    (QFP100)                  MB81C466-10 x4 |
|G                         (QFP100)                                             |
|A                                                                              |
|                                                                               |
|5                                                                              |
|6                                                                              |
|                       315-5294        315-5292                  315-5293      |
|                       (QFP100)        (QFP160)     32MHz        (QFP160)      |
|            MB81C78A-45                                                        |
|-|          MB81C78A-45                         M5M4464-12 x4   MB81461-12 x6  |
  |                                                                             |
|-|      YM3012                                                                 |
|                                                                               |
|             315-5242  HM65256 HM65256 HM65256  M5M4464-12 x4   MB81461-12 x6  |
|             (QFP44)   HM65256 HM65256 HM65256                                 |
|-------------------------------------------------------------------------------|


Floppy Controller PCB Layout
----------------------------


837-6443         ___________
|---------------|    DATA   |-- ||||---|
|                               PWR    |
|     74LS367 74LS05                   |
|     74LS367 74LS174                  |
|                     MB4107   74LS05  |
|                                      |
|     MB89311         74LS139   8MHz   |
|                CN1                   |
|     --------------------------|      |
|     |--------------------------      |
|--------------------------------------|


Notes:
            68000 clock: 10.000MHz
   Hitachi FD1094 clock: 10.000MHz
           YM2151 clock: 4.000MHz
                  VSync: 58Hz
                  HSync: 24.33kHz (game requires 24kHz monitor)
        CN2 (Above PCB): Connector for ROM Board (Not used for Gain Ground)
        CN2 (Below PCB): Connector for Floppy Controller Board
        PCB Pinout same as System 16
        Floppy Drive is a standard 1.44 High Density drive, but the controller
        is custom and the floppy disk format is custom. The floppy disk can be read with "Anadisk"
        depending on the PC being used and its floppy controller. Mostly, PC's can't read the System 24
        floppies even with "Anadisk"[1]

  [1] Actually, most can _except_ for the hotrod disks.  Those 8K sectors are deadly.
*/


/* system24temp_ functions / variables are from shared rewrite files,
   once the rest of the rewrite is complete they can be removed, I
   just made a copy & renamed them for now to avoid any conflicts
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68k.h"
#include "system24.h"
#include "system16.h"
#include "vidhrdw/segaic24.h"
#include "sound/ym2151.h"

VIDEO_START(system24);
VIDEO_UPDATE(system24);


/* Floppy Fisk Controller*/

static int fdc_status, fdc_track, fdc_sector, fdc_data;
static int fdc_phys_track, fdc_irq, fdc_drq, fdc_span, fdc_index_count;
static unsigned char *fdc_pt;
static int track_size;

static void fdc_init(void)
{
	fdc_status = 0;
	fdc_track = 0;
	fdc_sector = 0;
	fdc_data = 0;
	fdc_phys_track = 0;
	fdc_irq = 0;
	fdc_drq = 0;
	fdc_index_count = 0;
}

static READ16_HANDLER( fdc_r )
{
	if(!track_size)
		return 0xffff;

	switch(offset) {
	case 0:
		fdc_irq = 0;
		return fdc_status;
	case 1:
		return fdc_track;
	case 2:
		return fdc_sector;
	case 3:
	default: {
		int res = fdc_data;
		if(fdc_drq) {
			fdc_span--;
			/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "Read %02x (%d)\n", res, fdc_span);*/
			if(fdc_span) {
				fdc_pt++;
				fdc_data = *fdc_pt;
			} else {
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: transfert complete\n");
				fdc_drq = 0;
				fdc_status = 0;
				fdc_irq = 1;
			}
		} else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: data read with drq down\n");
		return res;
	}
	}
}

static WRITE16_HANDLER( fdc_w )
{
	if(!track_size)
		return;

	if(ACCESSING_LSB) {
		data &= 0xff;
		switch(offset) {
		case 0:
			fdc_irq = 0;
			switch(data >> 4) {
			case 0x0:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Restore\n");
				fdc_phys_track = fdc_track = 0;
				fdc_irq = 1;
				fdc_status = 4;
				break;
			case 0x1:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Seek %d\n", fdc_data);
				fdc_phys_track = fdc_track = fdc_data;
				fdc_irq = 1;
				fdc_status = fdc_track ? 0 : 4;
				break;
			case 0x9:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Read multiple [%02x] %d..%d side %d track %d\n", data, fdc_sector, fdc_sector+fdc_data-1, data & 8 ? 1 : 0, fdc_phys_track);
				fdc_pt = memory_region(REGION_USER2) + track_size*(2*fdc_phys_track+(data & 8 ? 1 : 0));
				fdc_span = track_size;
				fdc_status = 3;
				fdc_drq = 1;
				fdc_data = *fdc_pt;
				break;
			case 0xb:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Write multiple [%02x] %d..%d side %d track %d\n", data, fdc_sector, fdc_sector+fdc_data-1, data & 8 ? 1 : 0, fdc_phys_track);
				fdc_pt = memory_region(REGION_USER2) + track_size*(2*fdc_phys_track+(data & 8 ? 1 : 0));
				fdc_span = track_size;
				fdc_status = 3;
				fdc_drq = 1;
				break;
			case 0xd:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Forced interrupt\n");
				fdc_span = 0;
				fdc_drq = 0;
				fdc_irq = data & 1;
				fdc_status = 0;
				break;
			case 0xf:
				if(data == 0xfe)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Assign mode %02x\n", fdc_data);
				else if(data == 0xfd)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Assign parameter %02x\n", fdc_data);
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Unknown command %02x\n", data);
				break;
			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Unknown command %02x\n", data);
				break;
			}
			break;
		case 1:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Track register %02x\n", data);
			fdc_track = data;
			break;
		case 2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Sector register %02x\n", data);
			fdc_sector = data;
			break;
		case 3:
			if(fdc_drq) {
				/*				log_cb(RETRO_LOG_DEBUG, LOGPRE "Write %02x (%d)\n", data, fdc_span);*/
				*fdc_pt++ = data;
				fdc_span--;
				if(!fdc_span) {
					log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: transfert complete\n");
					fdc_drq = 0;
					fdc_status = 0;
					fdc_irq = 1;
				}
			} else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC: Data register %02x\n", data);
			fdc_data = data;
			break;
		}
	}
}

static READ16_HANDLER( fdc_status_r )
{
	if(!track_size)
		return 0xffff;

	return 0x90 | (fdc_irq ? 2 : 0) | (fdc_drq ? 1 : 0) | (fdc_phys_track ? 0x40 : 0) | (fdc_index_count ? 0x20 : 0);
}

static WRITE16_HANDLER( fdc_ctrl_w )
{
	if(ACCESSING_LSB)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "FDC control %02x\n", data & 0xff);
}


/* I/O Mappers*/

static UINT8 hotrod_io_r(int port)
{
	switch(port) {
	case 0:
		return readinputport(0);
	case 1:
		return readinputport(1);
	case 2:
		return 0xff;
	case 3:
		return 0xff;
	case 4:
		return readinputport(2);
	case 5: /* Dip switches*/
		return readinputport(3);
	case 6:
		return readinputport(4);
	case 7: /* DAC*/
		return 0xff;
	}
	return 0x00;
}

static UINT8 dcclub_io_r(int port)
{
	switch(port) {
	case 0: {
		static UINT8 pos[16] = { 0, 1, 3, 2, 6, 4, 12, 8, 9 };
		return (readinputport(0) & 0xf) | ((~pos[readinputport(5)>>4]<<4) & 0xf0);
	}
	case 1:
		return readinputport(1);
	case 2:
		return 0xff;
	case 3:
		return 0xff;
	case 4:
		return readinputport(2);
	case 5: /* Dip switches*/
		return readinputport(3);
	case 6:
		return readinputport(4);
	case 7: /* DAC*/
		return 0xff;
	}
	return 0x00;
}

static int cur_input_line;

static UINT8 mahmajn_io_r(int port)
{
	switch(port) {
	case 0:
		return ~(1 << cur_input_line);
	case 1:
		return 0xff;
	case 2:
		return readinputport(cur_input_line);
	case 3:
		return 0xff;
	case 4:
		return readinputport(8);
	case 5: /* Dip switches*/
		return readinputport(9);
	case 6:
		return readinputport(10);
	case 7: /* DAC*/
		return 0xff;
	}
	return 0x00;
}

static void mahmajn_io_w(int port, UINT8 data)
{
	switch(port) {
	case 3:
		if(data & 4)
			cur_input_line = (cur_input_line + 1) & 7;
		break;
	case 7: /* DAC*/
		DAC_0_signed_data_w(0, data);
		break;
	default:
		fprintf(stderr, "Port %d : %02x\n", port, data & 0xff);
	}
}

static void hotrod_io_w(int port, UINT8 data)
{
	switch(port) {
	case 3: /* Lamps*/
		break;
	case 7: /* DAC*/
		DAC_0_signed_data_w(0, data);
		break;
	default:
		fprintf(stderr, "Port %d : %02x\n", port, data & 0xff);
	}
}

static UINT8 hotrod_ctrl_cur;

static WRITE16_HANDLER( hotrod3_ctrl_w )
{
	if(ACCESSING_LSB) {
		data &= 3;
		if(data == 3)
			hotrod_ctrl_cur = 0;
		else
			hotrod_ctrl_cur = readinputport(8+data);
	}
}

static READ16_HANDLER( hotrod3_ctrl_r )
{
	if(ACCESSING_LSB) {
		switch(offset) {
			/* Steering dials*/
		case 0:
			return readinputport(5) & 0xff;
		case 1:
			return readinputport(5) >> 8;
		case 2:
			return readinputport(6) & 0xff;
		case 3:
			return readinputport(6) >> 8;
		case 4:
			return readinputport(7) & 0xff;
		case 5:
			return readinputport(7) >> 8;

		case 6:
			return 0xff;
		case 7:
			return 0xff;

		case 8: { /* Serial ADCs for the accel*/
			int v = hotrod_ctrl_cur & 0x80;
			hotrod_ctrl_cur <<= 1;
			return v ? 0xff : 0;
		}
		}
	}
	return 0;
}

static READ16_HANDLER( iod_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "IO daughterboard read %02x (%x)\n", offset, activecpu_get_pc());
	return 0xffff;
}

static WRITE16_HANDLER( iod_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "IO daughterboard write %02x, %04x & %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}


/* Cpu #1 reset control*/

static unsigned char resetcontrol, prev_resetcontrol;

static void reset_reset(void)
{
	int changed = resetcontrol ^ prev_resetcontrol;
	if(changed & 2) {
		if(resetcontrol & 2) {
			cpu_set_halt_line(1, CLEAR_LINE);
			cpu_set_reset_line(1, PULSE_LINE);
		} else
			cpu_set_halt_line(1, ASSERT_LINE);
	}
	if(changed & 4)
		YM2151ResetChip(0);
	prev_resetcontrol = resetcontrol;
}

static void resetcontrol_w(UINT8 data)
{
	resetcontrol = data;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Reset control %02x (%x:%x)\n", resetcontrol, cpu_getactivecpu(), activecpu_get_pc());
	reset_reset();
}


/* Rom board bank access*/

static unsigned char curbank;
static const UINT16 *rom_base;

static void reset_bank(void)
{
	rom_base = (UINT16 *)(memory_region(REGION_USER1) + curbank * 0x40000);
}

static READ16_HANDLER( curbank_r )
{
	return curbank;
}

static WRITE16_HANDLER( curbank_w )
{
	if(ACCESSING_LSB) {
		curbank = data & 0xff;
		reset_bank();
	}
}


static READ16_HANDLER(rombank_r)
{
	return rom_base[offset];
}


/* Shared banks access*/

static UINT16 *ramlo, *ramhi, *ramprg;

static READ16_HANDLER( ramlo_r )
{
	return ramlo[offset];
}

static WRITE16_HANDLER( ramlo_w )
{
	COMBINE_DATA(ramlo+offset);
}

static READ16_HANDLER( ramhi_r )
{
	return ramhi[offset];
}

static WRITE16_HANDLER( ramhi_w )
{
	COMBINE_DATA(ramhi+offset);
}

static READ16_HANDLER( ramprg_r )
{
	return ramprg[offset];
}

static WRITE16_HANDLER( ramprg_w )
{
	COMBINE_DATA(ramprg+offset);
}

static READ16_HANDLER( rom_r )
{
	return ((UINT16 *)memory_region(REGION_CPU1))[offset];
}


/* YM2151*/

static READ16_HANDLER( ym_status_r )
{
	return YM2151_status_port_0_r(0);
}

static WRITE16_HANDLER( ym_register_w )
{
	if(ACCESSING_LSB)
		YM2151_register_port_0_w(0, data);
}

static WRITE16_HANDLER( ym_data_w )
{
	if(ACCESSING_LSB)
		YM2151_data_port_0_w(0, data);
}


/* Protection magic latch*/

static UINT8  mahmajn_mlt[8] = { 5, 1, 6, 2, 3, 7, 4, 0 };
static UINT8 mahmajn2_mlt[8] = { 6, 0, 5, 3, 1, 4, 2, 7 };
static UINT8      gqh_mlt[8] = { 3, 7, 4, 0, 2, 6, 5, 1 };
static UINT8 bnzabros_mlt[8] = { 2, 4, 0, 5, 7, 3, 1, 6 };
static UINT8   qrouka_mlt[8] = { 1, 6, 4, 7, 0, 5, 3, 2 };
static UINT8 quizmeku_mlt[8] = { 0, 3, 2, 4, 6, 1, 7, 5 };
static UINT8   dcclub_mlt[8] = { 4, 3, 7, 0, 2, 6, 1, 5 };

static UINT8 mlatch;
static const unsigned char *mlatch_table;

static READ16_HANDLER( mlatch_r )
{
	return mlatch;
}

static WRITE16_HANDLER( mlatch_w )
{
	if(ACCESSING_LSB) {
		int i;
		unsigned char mxor = 0;
		if(!mlatch_table) {
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Protection: magic latch accessed but no table loaded (%d:%x)\n", cpu_getactivecpu(), activecpu_get_pc());
			return;
		}

		data &= 0xff;

		if(data != 0xff) {
			for(i=0; i<8; i++)
				if(mlatch & (1<<i))
					mxor |= 1 << mlatch_table[i];
			mlatch = data ^ mxor;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Magic latching %02x ^ %02x as %02x (%d:%x)\n", data & 0xff, mxor, mlatch, cpu_getactivecpu(), activecpu_get_pc());
		} else {
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Magic latch reset (%d:%x)\n", cpu_getactivecpu(), activecpu_get_pc());
			mlatch = 0x00;
		}
	}
}


/* Timers and IRQs*/

enum {
	IRQ_YM2151 = 1,
	IRQ_TIMER  = 2,
	IRQ_VBLANK = 3,
	IRQ_SPRITE = 4
};

static UINT16 irq_timera;
static UINT8  irq_timerb;
static UINT8  irq_allow0, irq_allow1;
static int    irq_timer_pend0, irq_timer_pend1, irq_yms;
static void  *irq_timer;

static void irq_timer_cb(int param)
{
	irq_timer_pend0 = irq_timer_pend1 = 1;
	if(irq_allow0 & (1 << IRQ_TIMER))
		cpu_set_irq_line(0, IRQ_TIMER+1, ASSERT_LINE);
	if(irq_allow1 & (1 << IRQ_TIMER))
		cpu_set_irq_line(1, IRQ_TIMER+1, ASSERT_LINE);
}

static void irq_init(void)
{
	irq_timera = 0;
	irq_timerb = 0;
	irq_allow0 = 0;
	irq_allow1 = 0;
	irq_timer_pend0 = 0;
	irq_timer_pend1 = 0;
	irq_timer = timer_alloc(irq_timer_cb);
}

static void irq_timer_reset(void)
{
	int freq = (irq_timerb << 12) | irq_timera;
	freq &= 0x1fff;

	timer_adjust(irq_timer, TIME_IN_HZ(freq), 0, TIME_IN_HZ(freq));
	log_cb(RETRO_LOG_DEBUG, LOGPRE "New timer frequency: %0d [%02x %04x]\n", freq, irq_timerb, irq_timera);
}

static WRITE16_HANDLER(irq_w)
{
	switch(offset) {
	case 0: {
		UINT16 old_ta = irq_timera;
		COMBINE_DATA(&irq_timera);
		if(old_ta != irq_timera)
			irq_timer_reset();
		break;
	}
	case 1:
		if(ACCESSING_LSB) {
			UINT8 old_tb = irq_timerb;
			irq_timerb = data;
			if(old_tb != irq_timerb)
				irq_timer_reset();
		}
		break;
	case 2:
		irq_allow0 = data;
		cpu_set_irq_line(0, IRQ_TIMER+1, irq_timer_pend0 && (irq_allow0 & (1 << IRQ_TIMER)) ? ASSERT_LINE : CLEAR_LINE);
		cpu_set_irq_line(0, IRQ_YM2151+1, irq_yms && (irq_allow0 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
		break;
	case 3:
		irq_allow1 = data;
		cpu_set_irq_line(1, IRQ_TIMER+1, irq_timer_pend1 && (irq_allow1 & (1 << IRQ_TIMER)) ? ASSERT_LINE : CLEAR_LINE);
		cpu_set_irq_line(1, IRQ_YM2151+1, irq_yms && (irq_allow1 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
		break;
	}
}

static READ16_HANDLER(irq_r)
{
	switch(offset) {
	case 2:
		irq_timer_pend0 = 0;
		cpu_set_irq_line(0, IRQ_TIMER+1, CLEAR_LINE);
		break;
	case 3:
		irq_timer_pend1 = 0;
		cpu_set_irq_line(1, IRQ_TIMER+1, CLEAR_LINE);
		break;
	}
	return 0xffff;
}

static INTERRUPT_GEN(irq_vbl)
{
	int irq = cpu_getiloops() ? IRQ_SPRITE : IRQ_VBLANK;
	int mask = 1 << irq;

	if(irq_allow0 & mask)
		cpu_set_irq_line(0, 1+irq, HOLD_LINE);

	if(irq_allow1 & mask)
		cpu_set_irq_line(1, 1+irq, HOLD_LINE);

	if(!cpu_getiloops()) {
		/* Ensure one index pulse every 20 frames*/
		/* The is some code in bnzabros at 0x852 that makes it crash*/
		/* if the pulse train is too fast*/
		fdc_index_count++;
		if(fdc_index_count >= 20)
			fdc_index_count = 0;
	}
}

static void irq_ym(int irq)
{
	irq_yms = irq;
	cpu_set_irq_line(0, IRQ_YM2151+1, irq_yms && (irq_allow0 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
	cpu_set_irq_line(1, IRQ_YM2151+1, irq_yms && (irq_allow1 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
}



static MEMORY_READ16_START( system24_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x07ffff, rom_r },
	{ 0x080000, 0x0fffff, MRA16_RAM },
	{ 0x100000, 0x13ffff, rom_r },
	{ 0x140000, 0x17ffff, rom_r },
	{ 0x200000, 0x20ffff, sys24_tile_r },
	{ 0x280000, 0x29ffff, sys24_char_r },
	{ 0x400000, 0x403fff, MRA16_RAM },
	{ 0x404000, 0x40401f, sys24_mixer_r },
	{ 0x600000, 0x63ffff, sys24_sprite_r },
	{ 0x800000, 0x80007f, system24temp_sys16_io_r },
	{ 0x800102, 0x800103, ym_status_r },
	{ 0xa00000, 0xa00007, irq_r },

	{ 0xb00000, 0xb00007, fdc_r },
	{ 0xb00008, 0xb0000f, fdc_status_r },

	{ 0xb80000, 0xbbffff, rombank_r },
	{ 0xbc0000, 0xbc0001, curbank_r },
	{ 0xbc0006, 0xbc0007, mlatch_r },

	{ 0xc00000, 0xc00011, hotrod3_ctrl_r },

	{ 0xc80000, 0xcbffff, rombank_r },
	{ 0xcc0000, 0xcc0001, curbank_r },
	{ 0xcc0006, 0xcc0007, mlatch_r },

	{ 0xf00000, 0xf3ffff, ramprg_r },
	{ 0xfc0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( system24_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x0fffff, MWA16_RAM, &ramlo },
	{ 0x200000, 0x20ffff, sys24_tile_w },
	{ 0x220000, 0x220001, MWA16_NOP }, /* Unknown, always 0*/
	{ 0x240000, 0x240001, MWA16_NOP }, /* Horizontal synchronization register*/
	{ 0x260000, 0x260001, MWA16_NOP }, /* Vertical synchronization register*/
	{ 0x270000, 0x270001, MWA16_NOP }, /* Video synchronization switch*/
	{ 0x280000, 0x29ffff, sys24_char_w },
	{ 0x400000, 0x403fff, system24temp_sys16_paletteram1_w, &paletteram16 },
	{ 0x404000, 0x40401f, sys24_mixer_w },
	{ 0x600000, 0x63ffff, sys24_sprite_w },
	{ 0x800000, 0x80007f, system24temp_sys16_io_w },
	{ 0x800100, 0x800101, ym_register_w },
	{ 0x800102, 0x800103, ym_data_w },
	{ 0xa00000, 0xa00007, irq_w },

	{ 0xb00000, 0xb00007, fdc_w },
	{ 0xb00008, 0xb0000f, fdc_ctrl_w },

	{ 0xbc0000, 0xbc0001, curbank_w },
	{ 0xbc0006, 0xbc0007, mlatch_w },

	{ 0xc00010, 0xc00011, hotrod3_ctrl_w },

	{ 0xcc0000, 0xcc0001, curbank_w },
	{ 0xcc0006, 0xcc0007, mlatch_w },

{ 0xd00300, 0xd00301, MWA16_NOP },
	{ 0xf00000, 0xf3ffff, ramprg_w },
	{ 0xfc0000, 0xffffff, MWA16_RAM, &ramhi },
MEMORY_END

static MEMORY_READ16_START( system24_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_RAM },
	{ 0x040000, 0x07ffff, rom_r },
	{ 0x080000, 0x0fffff, ramlo_r },
	{ 0x100000, 0x13ffff, rom_r },
	{ 0x140000, 0x17ffff, rom_r },
	{ 0x200000, 0x20ffff, sys24_tile_r },
	{ 0x280000, 0x29ffff, sys24_char_r },
	{ 0x400000, 0x403fff, paletteram16_word_r },
	{ 0x404000, 0x40401f, sys24_mixer_r },
	{ 0x600000, 0x63ffff, sys24_sprite_r },
	{ 0x800000, 0x80007f, system24temp_sys16_io_r },
	{ 0x800102, 0x800103, ym_status_r },
	{ 0xa00000, 0xa00007, irq_r },

	{ 0xb80000, 0xbbffff, rombank_r },
	{ 0xbc0000, 0xbc0001, curbank_r },
	{ 0xbc0006, 0xbc0007, mlatch_r },

	{ 0xc00000, 0xc00011, hotrod3_ctrl_r },

	{ 0xc80000, 0xcbffff, rombank_r },
	{ 0xcc0000, 0xcc0001, curbank_r },
	{ 0xcc0006, 0xcc0007, mlatch_r },

	{ 0xf00000, 0xf3ffff, ramprg_r },
	{ 0xfc0000, 0xffffff, ramhi_r },
MEMORY_END

static MEMORY_WRITE16_START( system24_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_RAM, &ramprg },
	{ 0x080000, 0x0fffff, ramlo_w },
	{ 0x200000, 0x20ffff, sys24_tile_w },
	{ 0x220000, 0x220001, MWA16_NOP }, /* Unknown, always 0*/
	{ 0x240000, 0x240001, MWA16_NOP }, /* Horizontal synchronization register*/
	{ 0x260000, 0x260001, MWA16_NOP }, /* Vertical synchronization register*/
	{ 0x270000, 0x270001, MWA16_NOP }, /* Video synchronization switch*/
	{ 0x280000, 0x29ffff, sys24_char_w },
	{ 0x400000, 0x403fff, system24temp_sys16_paletteram1_w },
	{ 0x404000, 0x40401f, sys24_mixer_w },
	{ 0x600000, 0x63ffff, sys24_sprite_w },
	{ 0x800000, 0x80007f, system24temp_sys16_io_w },
	{ 0x800100, 0x800101, ym_register_w },
	{ 0x800102, 0x800103, ym_data_w },
	{ 0xa00000, 0xa00007, irq_w },
	{ 0xbc0000, 0xbc0001, curbank_w },
	{ 0xbc0006, 0xbc0007, mlatch_w },
	{ 0xc00010, 0xc00011, hotrod3_ctrl_w },
	{ 0xcc0000, 0xcc0001, curbank_w },
	{ 0xcc0006, 0xcc0007, mlatch_w },
{ 0xd00300, 0xd00301, MWA16_NOP },
	{ 0xf00000, 0xf3ffff, ramprg_w },
	{ 0xfc0000, 0xffffff, ramhi_w },
MEMORY_END

static DRIVER_INIT(qgh)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = gqh_mlt;
	track_size = 0;
}

static DRIVER_INIT(dcclub)
{
	system24temp_sys16_io_set_callbacks(dcclub_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = dcclub_mlt;
	track_size = 0;
}

static DRIVER_INIT(qrouka)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = qrouka_mlt;
	track_size = 0;
}

static DRIVER_INIT(quizmeku)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = quizmeku_mlt;
	track_size = 0;
}

static DRIVER_INIT(mahmajn)
{

	system24temp_sys16_io_set_callbacks(mahmajn_io_r, mahmajn_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = mahmajn_mlt;
	track_size = 0;
	cur_input_line = 0;
}

static DRIVER_INIT(mahmajn2)
{

	system24temp_sys16_io_set_callbacks(mahmajn_io_r, mahmajn_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = mahmajn2_mlt;
	track_size = 0;
	cur_input_line = 0;
}

static DRIVER_INIT(hotrod)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;

	/* Sector  Size*/
	/* 1       8192*/
	/* 2       1024*/
	/* 3       1024*/
	/* 4       1024*/
	/* 5        512*/
	/* 6        256*/

	track_size = 0x2f00;
}

static DRIVER_INIT(bnzabros)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = bnzabros_mlt;

	/* Sector  Size*/
	/* 1       2048*/
	/* 2       2048*/
	/* 3       2048*/
	/* 4       2048*/
	/* 5       2048*/
	/* 6       1024*/
	/* 7        256*/

	track_size = 0x2d00;
}

static DRIVER_INIT(sspirits)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;
	track_size = 0x2d00;
}

static DRIVER_INIT(sgmast)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;
	track_size = 0x2d00;
}

static DRIVER_INIT(qsww)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;
	track_size = 0x2d00;
}

static DRIVER_INIT(gground)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;
	track_size = 0x2d00;
}

static DRIVER_INIT(crkdown)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;
	track_size = 0x2d00;
}

static NVRAM_HANDLER(system24)
{
	if(!track_size || !file)
		return;
	if(read_or_write)
		mame_fwrite(file, memory_region(REGION_USER2), 2*track_size);
	else
		mame_fread(file, memory_region(REGION_USER2), 2*track_size);
}

static MACHINE_INIT(system24)
{
	cpu_set_halt_line(1, ASSERT_LINE);
	prev_resetcontrol = resetcontrol = 0x06;
	fdc_init();
	curbank = 0;
	reset_bank();
	irq_init();
	mlatch = 0x00;
}

INPUT_PORTS_START( hotrod )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN6 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Start Credit" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Coin Chute" )
	PORT_DIPSETTING(    0x40, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0x80, 0x80, "Monitor flip" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_ANALOG( 0xfff, 0x000, IPT_DIAL | IPF_PLAYER1, 25, 15, 0x000, 0xfff )

	PORT_START
	PORT_ANALOG( 0xfff, 0x000, IPT_DIAL | IPF_PLAYER2, 25, 15, 0x000, 0xfff )

	PORT_START
	PORT_ANALOG( 0xfff, 0x000, IPT_DIAL | IPF_PLAYER3, 25, 15, 0x000, 0xfff )

	PORT_START
	PORT_ANALOG( 0xff, 0x01, IPT_PEDAL | IPF_PLAYER1, 25, 15, 0x01, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x01, IPT_PEDAL | IPF_PLAYER2, 25, 15, 0x01, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x01, IPT_PEDAL | IPF_PLAYER3, 25, 15, 0x01, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( bnzabros )
	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Start Credit" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Chute" )
	PORT_DIPSETTING(    0x04, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0x08, 0x08, "Monitor flip" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Initial lives" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x00, "1" )
INPUT_PORTS_END


INPUT_PORTS_START( qgh )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Monitor flip" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Initial lives" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( dcclub )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Start Credit" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x02, "Monitor flip" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Timing Meter" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPNAME( 0x10, 0x10, "Initial Balls" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x20, 0x20, "Balls Limit" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 16, 16, 0x00, 0x8f )
INPUT_PORTS_END

INPUT_PORTS_START( quizmeku )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Players" )
	PORT_DIPSETTING(    0x01, "2P" )
	PORT_DIPSETTING(    0x00, "4P" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Initial lives" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, "Answer Counts" )
	PORT_DIPSETTING(    0x10, "Every" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x20, "Monitor flip" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( mahmajn )
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Start score" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty (computer)" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty (player)" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

ROM_START( hotrod )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11339.bin", 0x000000, 0x20000, CRC(75130e73) SHA1(e079739f4a3da3807aac570442c5afef1a7d7b0e) )
	ROM_LOAD16_BYTE( "epr11338.bin", 0x000001, 0x20000, CRC(7d4a7ff3) SHA1(3d3af04d990d232ba0a8fe155de59bc632a0a461) )

	ROM_REGION( 0x1d6000, REGION_USER2, 0)
	ROM_LOAD( "ds3-5000-01d_3p_turbo.bin", 0x000000, 0x1d6000, CRC(627e8053) SHA1(d1a95f99078f5a29cccacfb1b30c3c9ead7b605c) )
ROM_END

ROM_START( qgh )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "16900b", 0x000000, 0x20000, CRC(20d7b7d1) SHA1(345b228c27e5f2fef9a2b8b5f619c59450a070f8) )
	ROM_LOAD16_BYTE( "16899b", 0x000001, 0x20000, CRC(397b3ba9) SHA1(1773212cd87dcff840f3953ec368be7e2394faf0) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "16902a", 0x000000, 0x80000, CRC(d35b7706) SHA1(341bca0af6b6d3f326328a88cdc69c7897b83a0d) )
	ROM_LOAD16_BYTE( "16901a", 0x000001, 0x80000, CRC(ab4bcb33) SHA1(8acd73096eb485c6dc83da6adfcc47d5d0f5b7f3) )
	ROM_LOAD16_BYTE( "16904",  0x100000, 0x80000, CRC(10987c88) SHA1(66f893690565aed613427421958ebe225a20ad0f) )
	ROM_LOAD16_BYTE( "16903",  0x100001, 0x80000, CRC(c19f9e46) SHA1(f1275674a8b44957428d79402f240ca21a34f48d) )
	ROM_LOAD16_BYTE( "16906",  0x200000, 0x80000, CRC(99c6773e) SHA1(568570b607d2cbbedb39ceae5bbc479478fae4ca) )
	ROM_LOAD16_BYTE( "16905",  0x200001, 0x80000, CRC(3922bbe3) SHA1(4378ca900f96138b5e33265ddac56af7b45afbc8) )
	ROM_LOAD16_BYTE( "16908",  0x300000, 0x80000, CRC(407ec20f) SHA1(c8a909d8e9ba024a37a5af6b7920fe7023f80d49) )
	ROM_LOAD16_BYTE( "16907",  0x300001, 0x80000, CRC(734b0a82) SHA1(d3fb31c55e79b99040beb7c49faaf2e17b95aa87) )
ROM_END

ROM_START( qrouka )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "14485", 0x000000, 0x20000, CRC(fc0085f9) SHA1(0250d1e17e19b541b85198ec4207e55bfbd5c32e) )
	ROM_LOAD16_BYTE( "14484", 0x000001, 0x20000, CRC(f51c641c) SHA1(3f2fd0be7d58c75e88565393da5e810655413b53) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "14482", 0x000000, 0x80000, CRC(7a13dd97) SHA1(bfe9950d2cd41f3f866520923c1ed7b8da1990ec) )
	ROM_LOAD16_BYTE( "14483", 0x100000, 0x80000, CRC(f3eb51a0) SHA1(6904830ff5e7aa5f016e115572fb6da678896ede) )
ROM_END

ROM_START( mahmajn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14813.bin", 0x000000, 0x20000, CRC(ea38cf4b) SHA1(118ab2e0ae20a4db5e619945dfbb3f200de3979c) )
	ROM_LOAD16_BYTE( "epr14812.bin", 0x000001, 0x20000, CRC(5a3cb4a7) SHA1(c0f21282140e8e6e927664f5f2b90525ae0207e9) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr14820.bin", 0x000000, 0x80000, CRC(8d2a03d3) SHA1(b3339bcd101bcfe042e2a1cfdc8baef0a86624df) )
	ROM_LOAD16_BYTE( "mpr14819.bin", 0x000001, 0x80000, CRC(e84c4827) SHA1(54741295e1bdca7d0c78eb795a68b92212d43b2e) )
	ROM_LOAD16_BYTE( "mpr14822.bin", 0x100000, 0x80000, CRC(7c3dcc51) SHA1(a199c2c71cda44a2c8755074c1007d83c8d45d2d) )
	ROM_LOAD16_BYTE( "mpr14821.bin", 0x100001, 0x80000, CRC(bd8dc543) SHA1(fd50b14fa73307a62dc0b522cfedb8b3332c407e) )
	ROM_LOAD16_BYTE( "mpr14824.bin", 0x200000, 0x80000, CRC(38311933) SHA1(237d9a8ffe14ba9ec371bb571d7c9e74a93fe1f3) )
	ROM_LOAD16_BYTE( "mpr14823.bin", 0x200001, 0x80000, CRC(4c8d4550) SHA1(be8717d4080ce932fc8272ebe54e2b0a60b20edd) )
	ROM_LOAD16_BYTE( "mpr14826.bin", 0x300000, 0x80000, CRC(c31b8805) SHA1(b446388c83af2e14300b0c4248470d3a8c504f2c) )
	ROM_LOAD16_BYTE( "mpr14825.bin", 0x300001, 0x80000, CRC(191080a1) SHA1(407c1c5fa4c76732e8a444860094542e90a1e8e8) )
ROM_END

ROM_START( mahmajn2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr16799.bin", 0x000000, 0x20000, CRC(3a34cf75) SHA1(d22bf6334668af29167cf4244d18f9cd2e7ff7d6) )
	ROM_LOAD16_BYTE( "epr16798.bin", 0x000001, 0x20000, CRC(662923fa) SHA1(dcd3964d899d3f34dab22ffcd1a5af895804fae1) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr16801.bin", 0x000000, 0x80000, CRC(74855a17) SHA1(d2d8e7da7b261e7cb64605284d2c78fbd1465b69) )
	ROM_LOAD16_BYTE( "mpr16800.bin", 0x000001, 0x80000, CRC(6dbc1e02) SHA1(cce5734243ff171759cecb5c05c12dc743a25c1d) )
	ROM_LOAD16_BYTE( "mpr16803.bin", 0x100000, 0x80000, CRC(9b658dd6) SHA1(eaaae289a3555aa6a92f57eea964dbbf48c5c2a4) )
	ROM_LOAD16_BYTE( "mpr16802.bin", 0x100001, 0x80000, CRC(b4723225) SHA1(acb8923c7d9908b1112f8d1f2512f18236915e5d) )
	ROM_LOAD16_BYTE( "mpr16805.bin", 0x200000, 0x80000, CRC(d15528df) SHA1(bda1dd5c98867c2e7666380bca0bc7eef8022fbc) )
	ROM_LOAD16_BYTE( "mpr16804.bin", 0x200001, 0x80000, CRC(a0de08e2) SHA1(2c36b66e74b88fb076e2eaa250c6d06ee0b4ac88) )
	ROM_LOAD16_BYTE( "mpr16807.bin", 0x300000, 0x80000, CRC(816188bb) SHA1(76b2690a6156766a1af94f01f6de1209b7756b2c) )
	ROM_LOAD16_BYTE( "mpr16806.bin", 0x300001, 0x80000, CRC(54b353d3) SHA1(40632e4571b44ee215b5a1f7aab9d89c460a5c9e) )
ROM_END

ROM_START( bnzabros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr13188.h2",  0x000000, 0x80000, CRC(d3802294) SHA1(7608e71e8ef398ac24dbf851994253bca5ace625) )
	ROM_LOAD16_BYTE( "mpr13187.h1",  0x000001, 0x80000, CRC(e3d8c5f7) SHA1(5b1e8646debee2f2ef272ddd3320b0a17192fbbe) )
	ROM_LOAD16_BYTE( "mpr13190.4",   0x100000, 0x40000, CRC(0b4df388) SHA1(340478bba82069ab745d6d8703e6801411fd2fc4) )
	ROM_RELOAD ( 0x180000, 0x40000)
	ROM_LOAD16_BYTE( "mpr13189.3",   0x100001, 0x40000, CRC(5ea5a2f3) SHA1(514b5446303c50aeb1d6d10d0a3f210da2577e16) )
	ROM_RELOAD ( 0x180001, 0x40000)

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "bb-disk.bin",        0x000000, 0x1c2000, CRC(ea7a3302) SHA1(5f92efb2e1135c1f3eeca38ba5789739a22dbd11) )
ROM_END

ROM_START( quizmeku ) /* Quiz Mekuromeku Story*/
	 ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	 ROM_LOAD16_BYTE( "epr15343.ic2", 0x000000, 0x20000, CRC(c72399a7) SHA1(bfbf0079ea63f89bca4ce9081aed5d5c1d9d169a) )
	 ROM_LOAD16_BYTE( "epr15342.ic1", 0x000001, 0x20000, CRC(0968ac84) SHA1(4e1170ac123adaec32819754b5075531ff1925fe) )

	 ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	 ROM_LOAD16_BYTE( "epr15345.ic5", 0x000000, 0x80000, CRC(88030b5d) SHA1(d2feeedb9a64c3dc8dd25716209f945d12fa9b53) )
	 ROM_LOAD16_BYTE( "epr15344.ic4", 0x000001, 0x80000, CRC(dd11b382) SHA1(2b0f49fb307a9aba0f295de64782ee095c557170) )
	 ROM_LOAD16_BYTE( "mpr15347.ic7", 0x100000, 0x80000, CRC(0472677b) SHA1(93ae57a2817b6b54c99814fca28ef51f7ff5e559) )
	 ROM_LOAD16_BYTE( "mpr15346.ic6", 0x100001, 0x80000, CRC(746d4d0e) SHA1(7863abe36126684772a4459d5b6f3b24670ec02b) )
	 ROM_LOAD16_BYTE( "mpr15349.ic9", 0x200000, 0x80000, CRC(770eecf1) SHA1(86cc5b4a325198dc1da1446ecd8e718415b7998a) )
	 ROM_LOAD16_BYTE( "mpr15348.ic8", 0x200001, 0x80000, CRC(7666e960) SHA1(f3f88d5c8318301a8c73141de60292f8349ac0ce) )
ROM_END

ROM_START( sspirits )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "ss-dump.bin",         0x000000, 0x1c2000, CRC(75d79c0c) SHA1(413ff2c10ce5e74d47da946fdd07eab14af53778) )
ROM_END

ROM_START( sgmast )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "sm-dump.bin",         0x000000, 0x1c2000, CRC(460bdcd5) SHA1(49b7384ac5742b45b7369f220f33f04ef955e992) )
ROM_END

ROM_START( qsww )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "ds3-5000-08b.bin",    0x000000, 0x1c2000, CRC(5a886d38) SHA1(2e974a9ffe3534da4fb117c579b8b0e61a63542c) )
ROM_END

ROM_START( gground )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "gg-dump.bin",         0x000000, 0x1c2000, CRC(5c5910f2) SHA1(9ed564a03c0d4ca4a207f3ecfb7336c6cbcaa70f) )
ROM_END

ROM_START( crkdown )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) SHA1(4b3b32df7de85aef9cd77c8a4ffc17e10466b638) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) SHA1(0ede61f0700f9161285c768fa97636f0e42b96f8) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "ds3-5000-04d.bin",    0x000000, 0x1c2000, CRC(b95bcdb7) SHA1(25c465349972ec5e57765ca6446883943daf3890) )
ROM_END

ROM_START( dcclub )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13948.bin", 0x000000, 0x20000, CRC(d6a031c8) SHA1(45b7e3cd2c7412e24f547cd4185166199d3938d5) )
	ROM_LOAD16_BYTE( "epr13947.bin", 0x000001, 0x20000, CRC(7e3cff5e) SHA1(ff8cb776d2491796feeb8892c7e644e590438945) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "epr15345.bin", 0x000000, 0x80000, CRC(d9e120c2) SHA1(b18b76733078d8534c6f0d8950632ab51e6a10ab) )
	ROM_LOAD16_BYTE( "epr15344.bin", 0x000001, 0x80000, CRC(8f8b9f74) SHA1(de6b923118bea60197547ad016cb5d5e1a8f372b) )
	ROM_LOAD16_BYTE( "mpr14097.bin", 0x100000, 0x80000, CRC(4bd74cae) SHA1(5aa90bd5d2b8e2338ef0fe41d1f794e8d51321e1) )
	ROM_LOAD16_BYTE( "mpr14096.bin", 0x100001, 0x80000, CRC(38d96502) SHA1(c68b3c5c83fd0839c3f6f81189c310ec19bdf1c4) )
ROM_END

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(50, MIXER_PAN_LEFT, 50, MIXER_PAN_RIGHT) },
	{ irq_ym }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static MACHINE_DRIVER_START( system24 )
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(system24_readmem, system24_writemem)
	MDRV_CPU_VBLANK_INT(irq_vbl, 2)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(system24_readmem2, system24_writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_INIT(system24)
	MDRV_NVRAM_HANDLER(system24)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192*2)

	MDRV_VIDEO_START(system24)
	MDRV_VIDEO_UPDATE(system24)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC,    dac_interface)
MACHINE_DRIVER_END


GAME( 1988, hotrod,   0, system24, hotrod,   hotrod,   ROT0, "Sega", "Hot Rod (turbo 3 player)")
GAME( 1990, bnzabros, 0, system24, bnzabros, bnzabros, ROT0, "Sega", "Bonanza Bros")
GAME( 1991, dcclub,   0, system24, dcclub,   dcclub,   ROT0, "Sega", "Dynamic Country Club")
GAME( 1992, mahmajn,  0, system24, mahmajn,  mahmajn,  ROT0, "Sega", "Tokoro San no MahMahjan")
GAME( 1994, qgh,      0, system24, qgh,      qgh,      ROT0, "Sega", "Quiz Ghost Hunter")
GAME( 1994, quizmeku, 0, system24, quizmeku, quizmeku, ROT0, "Sega", "Quiz Mekurumeku Story")
GAME( 1994, qrouka,   0, system24, qgh,      qrouka,   ROT0, "Sega", "Quiz Rouka Ni Tattenasai")
GAME( 1994, mahmajn2, 0, system24, mahmajn,  mahmajn2, ROT0, "Sega", "Tokoro San no MahMahjan 2")

/* Encrypted */
GAMEX( ????, sspirits, 0, system24, bnzabros, sspirits, ROT0, "Sega", "Scramble Spirits", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION)
GAMEX( ????, sgmast,   0, system24, bnzabros, sgmast,   ROT0, "Sega", "Super Masters Golf", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION)
GAMEX( ????, qsww,     0, system24, bnzabros, qsww,     ROT0, "Sega", "Quiz Syukudai wo Wasuremashita", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION)
GAMEX( ????, gground,  0, system24, bnzabros, gground,  ROT0, "Sega", "Gain Ground", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION)
GAMEX( ????, crkdown,  0, system24, bnzabros, crkdown,  ROT0, "Sega", "Crackdown", GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION)

/* Other S24 Games, mostly not dumped / encrypted / only bad disk images exist

Jumbo Ozaki Super Masters - Encrypted, Disk Based?
Scramble Spirits - Disk Based, Encrypted and Non-Encrypted versions Exist
+ a bunch of other Japanese Quiz Games

*/
