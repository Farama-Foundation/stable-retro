#define MW_DEBUG     0
#define MW_SKIPIDLE  1

/**************************************************************************
 * Mystic Warrior (c) 1993 Konami
 * Metamorphic Force (c) 1993 Konami
 * Violent Storm (c) 1993 Konami
 * Martial Champion (c) 1993 Konami
 * Gaiapolis (c) 1993 Konami
 * Ultimate Battler Dadandarn!! (c) 1993 Konami
 *
 * Driver by R. Belmont, Phil Stroffolino, Acho Tang, and Nicola Salmoria.
 * Assists from Olivier Galibert, Brian Troha, The Guru, and Yasuhiro Ogawa.
 *
 * These games are the "pre-GX" boards, combining features of the previous
 * line of hardware begun with Xexex and those of the future 32-bit System
 * GX (notably 5 bit per pixel graphics, the powerful K055555 mixer/priority
 * encoder, and K054338 alpha blend engine from System GX are used).
 *
 * Game status:
 * - All games are playable with sound and correct colors.
 * - Metamorphic Force's intro needs alpha blended sprites.
 */

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/konamigx.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"
#include "bootstrap.h"
#include "inptport.h"

VIDEO_START(gaiapols);
VIDEO_START(dadandrn);
VIDEO_START(viostorm);
VIDEO_START(metamrph);
VIDEO_START(martchmp);
VIDEO_START(mystwarr);
VIDEO_UPDATE(dadandrn);
VIDEO_UPDATE(mystwarr);
VIDEO_UPDATE(metamrph);
VIDEO_UPDATE(martchmp);

WRITE16_HANDLER(ddd_053936_enable_w);
WRITE16_HANDLER(ddd_053936_clip_w);
READ16_HANDLER(gai_053936_tilerom_0_r);
READ16_HANDLER(ddd_053936_tilerom_0_r);
READ16_HANDLER(ddd_053936_tilerom_1_r);
READ16_HANDLER(gai_053936_tilerom_2_r);
READ16_HANDLER(ddd_053936_tilerom_2_r);

static data16_t *gx_workram;

static int init_eeprom_count;
static int mw_irq_control;

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

/* Gaiapolis and Polygonet Commanders use the ER5911,
   but the command formats are slightly different.  Why? */
static struct EEPROM_interface eeprom_interface_gaia =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(mystwarr)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static NVRAM_HANDLER(gaiapols)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_gaia);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( mweeprom_r )
{
	if (ACCESSING_LSB)
	{
		int res = readinputport(1) | EEPROM_read_bit();

		if (init_eeprom_count)
		{
			init_eeprom_count--;
			res &= ~0x04;
		}

		return res;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "msb access to eeprom port\n");*/

	return 0;
}

static READ16_HANDLER( vseeprom_r )
{
	if (ACCESSING_LSB)
	{
		int res = readinputport(1) | EEPROM_read_bit();

		if (init_eeprom_count)
		{
			init_eeprom_count--;
			res &= ~0x08;
		}

		return res;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "msb access to eeprom port\n");*/

	return 0;
}

static WRITE16_HANDLER( mweeprom_w )
{
	if (ACCESSING_MSB)
	{
		EEPROM_write_bit((data&0x0100) ? 1 : 0);
		EEPROM_set_cs_line((data&0x0200) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data&0x0400) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown LSB write %x to eeprom\n", data);*/

}

static READ16_HANDLER( dddeeprom_r )
{
	if (ACCESSING_MSB)
	{
		return (readinputport(1) | EEPROM_read_bit())<<8;
	}

	return readinputport(3);
}

static WRITE16_HANDLER( mmeeprom_w )
{
	if (ACCESSING_LSB)
	{
		EEPROM_write_bit((data&0x01) ? 1 : 0);
		EEPROM_set_cs_line((data&0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}
}


/**********************************************************************************/
/* IRQ controllers */

#define ADD_SKIPPER16(PC, BASE, START, END, DATA, MASK){ \
  waitskip.pc   = PC;        \
  waitskip.offs = START/2;   \
  waitskip.data = DATA;      \
  waitskip.mask = MASK;      \
  resume_trigger= 1000;      \
  install_mem_read16_handler \
  (0, (BASE+START)&~1, (BASE+END)|1, waitskip_r);}

static struct { UINT32 offs, pc, mask, data; } waitskip;
static int suspension_active, resume_trigger;

static READ16_HANDLER(waitskip_r)
{
	data16_t data = gx_workram[waitskip.offs+offset];
	mem_mask = ~mem_mask;

	if (activecpu_get_pc() == waitskip.pc && (data & mem_mask) == (waitskip.data & mem_mask))
	{
		cpu_spinuntil_trigger(resume_trigger);
		suspension_active = 1;
	}

	return(data);
}


static INTERRUPT_GEN(mystwarr_interrupt)
{
	if (resume_trigger && suspension_active) { suspension_active = 0; cpu_trigger(resume_trigger); }

	if (!(mw_irq_control & 0x01)) return;

	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);
		break;

		case 1:
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
		break;

		case 2:
			cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
		break;
	}
}

static INTERRUPT_GEN(metamrph_interrupt)
{
	if (resume_trigger && suspension_active) { cpu_trigger(resume_trigger); suspension_active = 0; }

	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
		break;

		case 15:
			cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
		break;

		case 39:
			if (K053246_is_IRQ_enabled()) cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
		break;
	}
}

static INTERRUPT_GEN(mchamp_interrupt)
{
	if (resume_trigger && suspension_active) { cpu_trigger(resume_trigger); suspension_active = 0; }

	if (!(mw_irq_control & 0x02)) return;

	switch (cpu_getiloops())
	{
		case 0:
			if (K053246_is_IRQ_enabled()) cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
		break;

		case 1:
			cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);
		break;
	}
}

static INTERRUPT_GEN(ddd_interrupt)
{
	if (resume_trigger && suspension_active) { cpu_trigger(resume_trigger); suspension_active = 0; }

	cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
}


/**********************************************************************************/

static WRITE16_HANDLER( sound_cmd1_w )
{
	soundlatch_w(0, data&0xff);
}

static WRITE16_HANDLER( sound_cmd1_msb_w )
{
	soundlatch_w(0, data>>8);
}

static WRITE16_HANDLER( sound_cmd2_w )
{
	soundlatch2_w(0, data&0xff);
	return;
}

static WRITE16_HANDLER( sound_cmd2_msb_w )
{
	soundlatch2_w(0, data>>8);
	return;
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( sound_status_r )
{
	int latch = soundlatch3_r(0);

	if ((latch & 0xf) == 0xe) latch |= 1;

	return latch;
}

static READ16_HANDLER( sound_status_msb_r )
{
	int latch = soundlatch3_r(0);

	if ((latch & 0xf) == 0xe) latch |= 1;

	return latch<<8;
}

static WRITE16_HANDLER( irq_ack_w )
{
	K056832_b_word_w(offset, data, mem_mask);

	if (offset == 3 && ACCESSING_LSB)
	{
		mw_irq_control = data&0xff;

/*		if ((data &0xf0) != 0xd0) log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown write to IRQ reg: %x\n", data);*/

	}
}

static READ16_HANDLER( player1_r )
{
	return readinputport(2) | (readinputport(3)<<8);
}

static READ16_HANDLER( player2_r )
{
	return readinputport(4) | (readinputport(5)<<8);
}

static READ16_HANDLER( mmplayer1_r )
{
	return readinputport(2) | (readinputport(4)<<8);
}

static READ16_HANDLER( mmplayer2_r )
{
	return readinputport(3) | (readinputport(5)<<8);
}

static READ16_HANDLER( mmcoins_r )
{
	int res = readinputport(0);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= ~0x10;
	}

	return res;
}

static READ16_HANDLER( dddcoins_r )
{
	int res = (readinputport(0)<<8) | readinputport(2);

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= ~0x0800;
	}

	return res;
}

/* the interface with the 053247 is weird. The chip can address only 0x1000 bytes */
/* of RAM, but they put 0x10000 there. The CPU can access them all. */
static READ16_HANDLER( K053247_scattered_word_r )
{
	if (offset & 0x0078)
		return spriteram16[offset];
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x7f80) >> 4);
		return K053247_word_r(offset,mem_mask);
	}
}

static WRITE16_HANDLER( K053247_scattered_word_w )
{
	if (offset & 0x0078)
	{
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "spr write %x to %x (PC=%x)\n", data, offset, activecpu_get_pc());*/
		COMBINE_DATA(spriteram16+offset);
	}
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x7f80) >> 4);

		K053247_word_w(offset,data,mem_mask);
	}
}

/* 68000 memory handlers */
/* Mystic Warriors */
static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },	/* main program*/
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x482000, 0x48200f, K055673_rom_word_r },
	{ 0x494000, 0x494001, player1_r },
	{ 0x494002, 0x494003, player2_r },
	{ 0x496000, 0x496001, mmcoins_r },
	{ 0x496002, 0x496003, mweeprom_r },
	{ 0x498014, 0x498015, sound_status_r },
	{ 0x498000, 0x49801f, MRA16_RAM },
	{ 0x600000, 0x601fff, K056832_ram_word_r },
	{ 0x602000, 0x603fff, K056832_ram_word_r },	/* tilemap RAM mirror read(essential)*/
	{ 0x680000, 0x683fff, K056832_rom_word_r },
	{ 0x700000, 0x701fff, MRA16_RAM },
#if MW_DEBUG
	{ 0x480000, 0x4800ff, K055555_word_r },
	{ 0x482010, 0x48201f, K053247_reg_word_r },
	{ 0x484000, 0x484007, K053246_reg_word_r },
	{ 0x48a000, 0x48a01f, K054338_word_r },
	{ 0x48c000, 0x48c03f, K056832_word_r },
	{ 0x49c000, 0x49c01f, K053252_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x200000, 0x20ffff, MWA16_RAM, &gx_workram },
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x480000, 0x4800ff, K055555_word_w },
	{ 0x482010, 0x48201f, K053247_reg_word_w },
	{ 0x484000, 0x484007, K053246_word_w },
	{ 0x48a000, 0x48a01f, K054338_word_w },
	{ 0x48c000, 0x48c03f, K056832_word_w },
	{ 0x490000, 0x490001, mweeprom_w },
	{ 0x492000, 0x492001, MWA16_NOP },	/* watchdog*/
	{ 0x49800c, 0x49800d, sound_cmd1_w },
	{ 0x49800e, 0x49800f, sound_cmd2_w },
	{ 0x498000, 0x49801f, MWA16_RAM },
	{ 0x49a000, 0x49a001, sound_irq_w },
	{ 0x49c000, 0x49c01f, K053252_word_w },
	{ 0x49e000, 0x49e007, irq_ack_w },	/* VSCCS (custom)*/
	{ 0x600000, 0x601fff, K056832_ram_word_w },
	{ 0x602000, 0x603fff, K056832_ram_word_w },	/* tilemap RAM mirror write(essential)*/
	{ 0x700000, 0x701fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

/* Metamorphic Force */
static MEMORY_READ16_START( mmreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },	/* main program*/
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_r },
	{ 0x211000, 0x21ffff, MRA16_RAM },
	{ 0x244000, 0x24400f, K055673_rom_word_r },
 	{ 0x24c000, 0x24ffff, K053250_0_ram_r },
	{ 0x250000, 0x25000f, K053250_0_r },
	{ 0x268014, 0x268015, sound_status_r },
	{ 0x268000, 0x26801f, MRA16_RAM },
	{ 0x274000, 0x274001, mmplayer1_r },
	{ 0x274002, 0x274003, mmplayer2_r },
	{ 0x278000, 0x278001, mmcoins_r },
	{ 0x278002, 0x278003, vseeprom_r },
	{ 0x27c000, 0x27c001, MRA16_NOP },	/* watchdog lives here*/
	{ 0x300000, 0x301fff, K056832_ram_word_r },
	{ 0x302000, 0x303fff, K056832_ram_word_r },	/* tilemap RAM mirror read(essential)*/
	{ 0x310000, 0x311fff, K056832_rom_word_r },
	{ 0x320000, 0x321fff, K053250_0_rom_r },
	{ 0x330000, 0x331fff, MRA16_RAM },
#if MW_DEBUG
	{ 0x240000, 0x240007, K053246_reg_word_r },
	{ 0x244010, 0x24401f, K053247_reg_word_r },
	{ 0x254000, 0x25401f, K054338_word_r },
	{ 0x258000, 0x2580ff, K055555_word_r },
	{ 0x260000, 0x26001f, K053252_word_r },
	{ 0x26C000, 0x26C007, K056832_b_word_r },
	{ 0x270000, 0x27003f, K056832_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( mmwritemem )
	{ 0x200000, 0x20ffff, MWA16_RAM, &gx_workram },
	{ 0x210000, 0x210fff, K053247_word_w },
	{ 0x211000, 0x21ffff, MWA16_RAM },
	{ 0x240000, 0x240007, K053246_word_w },
	{ 0x244010, 0x24401f, K053247_reg_word_w },
 	{ 0x24c000, 0x24ffff, K053250_0_ram_w },	/* "LVC RAM" (53250_ram)*/
	{ 0x250000, 0x25000f, K053250_0_w },
	{ 0x254000, 0x25401f, K054338_word_w },
	{ 0x258000, 0x2580ff, K055555_word_w },
	{ 0x260000, 0x26001f, K053252_word_w },
	{ 0x264000, 0x264001, sound_irq_w },
	{ 0x26800c, 0x26800d, sound_cmd1_w },
	{ 0x26800e, 0x26800f, sound_cmd2_w },
	{ 0x268000, 0x26801f, MWA16_RAM },
	{ 0x26C000, 0x26C007, K056832_b_word_w },
	{ 0x270000, 0x27003f, K056832_word_w },
	{ 0x27C000, 0x27C001, mmeeprom_w },
	{ 0x300000, 0x301fff, K056832_ram_word_w },
	{ 0x302000, 0x303fff, K056832_ram_word_w },	/* tilemap RAM mirror write(essential)*/
	{ 0x330000, 0x331fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

/* Violent Storm */
static MEMORY_READ16_START( vsreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },		/* main program*/
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x210000, 0x210fff, K053247_word_r },
	{ 0x211000, 0x21ffff, MRA16_RAM },
	{ 0x244000, 0x24400f, K055673_rom_word_r },
 	{ 0x24c000, 0x24ffff, MRA16_RAM },		/* K053250_0_ram_r*/
	{ 0x250000, 0x25000f, MRA16_RAM },		/* K053250_0_r*/
	{ 0x25c000, 0x25c03f, K055550_word_r },
	{ 0x268014, 0x268015, sound_status_r },
	{ 0x268000, 0x26801f, MRA16_RAM },
	{ 0x274000, 0x274001, mmplayer1_r },
	{ 0x274002, 0x274003, mmplayer2_r },
	{ 0x278000, 0x278001, mmcoins_r },
	{ 0x278002, 0x278003, vseeprom_r },
	{ 0x27c000, 0x27c001, MRA16_NOP },		/* watchdog lives here*/
	{ 0x300000, 0x301fff, K056832_ram_word_r },
	{ 0x302000, 0x303fff, K056832_ram_word_r }, /* tilemap RAM mirror read(essential)*/
	{ 0x304000, 0x305fff, K056832_ram_word_r }, /* tilemap RAM mirror read(essential)*/
	{ 0x310000, 0x311fff, K056832_rom_word_r },
	{ 0x330000, 0x331fff, MRA16_RAM },
#if MW_DEBUG
	{ 0x240000, 0x240007, K053246_reg_word_r },
	{ 0x244010, 0x24401f, K053247_reg_word_r },
	{ 0x254000, 0x25401f, K054338_word_r },
	{ 0x258000, 0x2580ff, K055555_word_r },
	{ 0x260000, 0x26001f, K053252_word_r },
	{ 0x26C000, 0x26C007, K056832_b_word_r },
	{ 0x270000, 0x27003f, K056832_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( vswritemem )
	{ 0x200000, 0x20ffff, MWA16_RAM, &gx_workram },
	{ 0x210000, 0x210fff, K053247_word_w },
	{ 0x211000, 0x21ffff, MWA16_RAM },
	{ 0x240000, 0x240007, K053246_word_w },
	{ 0x244010, 0x24401f, K053247_reg_word_w },
 	{ 0x24c000, 0x24ffff, MWA16_RAM },			/* K053250_0_ram_w*/
	{ 0x250000, 0x25000f, MWA16_RAM },			/* K053250_0_w*/
	{ 0x254000, 0x25401f, K054338_word_w },
	{ 0x258000, 0x2580ff, K055555_word_w },
	{ 0x25c000, 0x25c03f, K055550_word_w },
	{ 0x260000, 0x26001f, K053252_word_w },
	{ 0x264000, 0x264001, sound_irq_w },
	{ 0x26800c, 0x26800d, sound_cmd1_w },
	{ 0x26800e, 0x26800f, sound_cmd2_w },
	{ 0x268000, 0x26801f, MWA16_RAM },
	{ 0x26C000, 0x26C007, K056832_b_word_w },
	{ 0x270000, 0x27003f, K056832_word_w },
	{ 0x27C000, 0x27C001, mmeeprom_w },
	{ 0x300000, 0x301fff, K056832_ram_word_w },
	{ 0x302000, 0x303fff, K056832_ram_word_w }, /* tilemap RAM mirror write(essential)*/
	{ 0x304000, 0x305fff, K056832_ram_word_w }, /* tilemap RAM mirror write(essential)*/
	{ 0x330000, 0x331fff, paletteram16_xrgb_word_w, &paletteram16 },
MEMORY_END

/* Martial Champion specific interfaces*/
static READ16_HANDLER( K053247_martchmp_word_r )
{
	if (offset & 0x0018)
		return spriteram16[offset];
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x1fe0) >> 2);
		return K053247_word_r(offset,mem_mask);
	}
}

static WRITE16_HANDLER( K053247_martchmp_word_w )
{
	if (offset & 0x0018)
	{
		COMBINE_DATA(spriteram16+offset);
	}
	else
	{
		offset = (offset & 0x0007) | ((offset & 0x1fe0) >> 2);

		K053247_word_w(offset,data,mem_mask);
	}
}

static READ16_HANDLER( mccontrol_r )
{
	return mw_irq_control<<8;
}

static WRITE16_HANDLER( mccontrol_w )
{
	if (ACCESSING_MSB)
	{
		mw_irq_control = data>>8;
		/* bit 0 = watchdog*/
		/* bit 1 = IRQ enable*/
		/* bit 2 = OBJCHA*/

		K053246_set_OBJCHA_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);

/*		if (data & 0xf8) log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk write %x to mccontrol\n", data);*/

	}

/*	else log_cb(RETRO_LOG_DEBUG, LOGPRE "write %x to LSB of mccontrol\n", data);*/

}

/* Martial Champion */
static MEMORY_READ16_START( mcreadmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* main program*/
	{ 0x100000, 0x10ffff, MRA16_RAM },			/* work RAM*/
	{ 0x300000, 0x3fffff, MRA16_ROM },			/* data ROM*/
	{ 0x402000, 0x40200f, K055673_rom_word_r },	/* sprite ROM readback*/
	{ 0x412000, 0x412001, mccontrol_r },
	{ 0x414000, 0x414001, player1_r },
	{ 0x414002, 0x414003, player2_r },
	{ 0x416000, 0x416001, mmcoins_r },			/* coin*/
	{ 0x416002, 0x416003, mweeprom_r },			/* eeprom read*/
	{ 0x418014, 0x418015, sound_status_r },		/* z80 status*/
	{ 0x418000, 0x41801f, MRA16_RAM },			/* sound regs fall through*/
	{ 0x480000, 0x483fff, K053247_martchmp_word_r },/* sprite RAM*/
	{ 0x600000, 0x601fff, MRA16_RAM },			/* palette RAM*/
	{ 0x680000, 0x681fff, K056832_ram_word_r },	/* tilemap RAM*/
	{ 0x682000, 0x683fff, K056832_ram_word_r },	/* tilemap RAM mirror read(essential)*/
	{ 0x700000, 0x703fff, K056832_rom_word_r },	/* tile ROM readback*/
#if MW_DEBUG
	{ 0x400000, 0x4000ff, K055555_word_r },
	{ 0x402010, 0x40201f, K053247_reg_word_r },
	{ 0x404000, 0x404007, K053246_reg_word_r },
	{ 0x40a000, 0x40a01f, K054338_word_r },
	{ 0x40c000, 0x40c03f, K056832_word_r },
	{ 0x41c000, 0x41c01f, K053252_word_r },
	{ 0x41e000, 0x41e007, K056832_b_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( mcwritemem )
	{ 0x100000, 0x10ffff, MWA16_RAM, &gx_workram },
	{ 0x400000, 0x4000ff, K055555_word_w },		/* PCU2*/
	{ 0x402010, 0x40201f, K053247_reg_word_w },	/* OBJSET2*/
	{ 0x404000, 0x404007, K053246_word_w },		/* OBJSET1*/
	{ 0x40a000, 0x40a01f, K054338_word_w },		/* CLTC*/
	{ 0x40c000, 0x40c03f, K056832_word_w },		/* VACSET*/
	{ 0x40e000, 0x40e03f, K053990_martchmp_word_w }, /* protection*/
	{ 0x410000, 0x410001, mweeprom_w },
	{ 0x412000, 0x412001, mccontrol_w },
	{ 0x41c000, 0x41c01f, K053252_word_w },		/* CCU*/
	{ 0x41e000, 0x41e007, K056832_b_word_w },	/* VSCCS*/
	{ 0x41800c, 0x41800d, sound_cmd1_w },
	{ 0x41800e, 0x41800f, sound_cmd2_w },
	{ 0x418000, 0x41801f, MWA16_RAM },			/* sound regs fall through*/
	{ 0x41a000, 0x41a001, sound_irq_w },
	{ 0x480000, 0x483fff, K053247_martchmp_word_w, &spriteram16 },
	{ 0x600000, 0x601fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x680000, 0x681fff, K056832_ram_word_w },
	{ 0x682000, 0x683fff, K056832_ram_word_w },	/* tilemap RAM mirror write(essential)*/
MEMORY_END

/* Ultimate Battler Dadandarn */
static MEMORY_READ16_START( dddreadmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },	/* main program and data ROM*/
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x410000, 0x411fff, K056832_ram_word_r },	/* tilemap RAM*/
	{ 0x412000, 0x413fff, K056832_ram_word_r }, /* tilemap RAM mirror read(essential)*/
	{ 0x420000, 0x421fff, MRA16_RAM },
	{ 0x440000, 0x443fff, K056832_rom_word_r },
	{ 0x450000, 0x45000f, K055673_rom_word_r },
	{ 0x470000, 0x470fff, MRA16_RAM },
	{ 0x48a014, 0x48a015, sound_status_msb_r },
	{ 0x48a000, 0x48a01f, MRA16_RAM },	/* sound regs fall-through*/
	{ 0x48e000, 0x48e001, dddcoins_r },	/* bit 3 (0x8) is test switch*/
	{ 0x48e020, 0x48e021, dddeeprom_r },
	{ 0x600000, 0x60ffff, MRA16_RAM },
	{ 0x680000, 0x68003f, K055550_word_r },
	{ 0x800000, 0x87ffff, ddd_053936_tilerom_0_r },	/* 256k tilemap readback*/
	{ 0xa00000, 0xa7ffff, ddd_053936_tilerom_1_r }, /* 128k tilemap readback*/
	{ 0xc00000, 0xdfffff, ddd_053936_tilerom_2_r },	/* tile character readback*/
#if MW_DEBUG
	{ 0x430000, 0x430007, K053246_reg_word_r },
	{ 0x450010, 0x45001f, K053247_reg_word_r },
	{ 0x480000, 0x48003f, K056832_word_r },
	{ 0x482000, 0x482007, K056832_b_word_r },
	{ 0x486000, 0x48601f, K053252_word_r },
	{ 0x488000, 0x4880ff, K055555_word_r },
	{ 0x48c000, 0x48c01f, K054338_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( dddwritemem )
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x410000, 0x411fff, K056832_ram_word_w },
	{ 0x412000, 0x413fff, K056832_ram_word_w },	/* tile RAM mirror write(essential)*/
	{ 0x420000, 0x421fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x430000, 0x430007, K053246_word_w },
	{ 0x450010, 0x45001f, K053247_reg_word_w },
	{ 0x460000, 0x46001f, MWA16_RAM, &K053936_0_ctrl },
	{ 0x470000, 0x470fff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x480000, 0x48003f, K056832_word_w },		/* VACSET*/
	{ 0x482000, 0x482007, K056832_b_word_w },	/* VSCCS*/
	{ 0x484000, 0x484003, ddd_053936_clip_w },
	{ 0x486000, 0x48601f, K053252_word_w },
	{ 0x488000, 0x4880ff, K055555_word_w },
	{ 0x48a00c, 0x48a00d, sound_cmd1_msb_w },
	{ 0x48a00e, 0x48a00f, sound_cmd2_msb_w },
	{ 0x48a000, 0x48a01f, MWA16_RAM },	/* sound regs fall-through*/
	{ 0x48c000, 0x48c01f, K054338_word_w },
	{ 0x600000, 0x60ffff, MWA16_RAM, &gx_workram },
	{ 0x680000, 0x68003f, K055550_word_w },
	{ 0x6a0000, 0x6a0001, mmeeprom_w },
	{ 0x6c0000, 0x6c0001, ddd_053936_enable_w },
	{ 0x6e0000, 0x6e0001, sound_irq_w },
	{ 0xe00000, 0xe00001, MWA16_NOP },	/* watchdog*/
MEMORY_END

/* Gaiapolis */
/* a00000 = the 128k tilemap*/
/* 800000 = the 256k tilemap*/
/* c00000 = 936 tiles (7fffff window)*/
static MEMORY_READ16_START( gaiareadmem )
	{ 0x000000, 0x2fffff, MRA16_ROM },	/* main program*/
	{ 0x400000, 0x40ffff, K053247_scattered_word_r },
	{ 0x410000, 0x411fff, K056832_ram_word_r },	/* tilemap RAM*/
	{ 0x412000, 0x413fff, K056832_ram_word_r }, /* tilemap RAM mirror read(essential)*/
	{ 0x420000, 0x421fff, MRA16_RAM },
	{ 0x440000, 0x441fff, K056832_rom_word_r },
	{ 0x450000, 0x45000f, K055673_rom_word_r },
	{ 0x470000, 0x470fff, MRA16_RAM },
	{ 0x48a014, 0x48a015, sound_status_msb_r },
	{ 0x48a000, 0x48a01f, MRA16_RAM },	/* sound regs fall-through*/
	{ 0x48e000, 0x48e001, dddcoins_r },	/* bit 3 (0x8) is test switch*/
	{ 0x48e020, 0x48e021, dddeeprom_r },
	{ 0x600000, 0x60ffff, MRA16_RAM },
	{ 0x660000, 0x6600ff, K054000_lsb_r },
	{ 0x800000, 0x87ffff, gai_053936_tilerom_0_r },	/* 256k tilemap readback*/
	{ 0xa00000, 0xa7ffff, ddd_053936_tilerom_1_r }, /* 128k tilemap readback*/
	{ 0xc00000, 0xdfffff, gai_053936_tilerom_2_r },	/* tile character readback*/
#if MW_DEBUG
	{ 0x430000, 0x430007, K053246_reg_word_r },
	{ 0x450010, 0x45001f, K053247_reg_word_r },
	{ 0x480000, 0x48003f, K056832_word_r },
	{ 0x482000, 0x482007, K056832_b_word_r },
	{ 0x486000, 0x48601f, K053252_word_r },
	{ 0x488000, 0x4880ff, K055555_word_r },
	{ 0x48c000, 0x48c01f, K054338_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( gaiawritemem )
	{ 0x400000, 0x40ffff, K053247_scattered_word_w, &spriteram16 },
	{ 0x410000, 0x411fff, K056832_ram_word_w },
	{ 0x412000, 0x413fff, K056832_ram_word_w },	/* tilemap RAM mirror write(essential)*/
	{ 0x412000, 0x4120ff, MWA16_RAM },
	{ 0x420000, 0x421fff, paletteram16_xrgb_word_w, &paletteram16 },
	{ 0x430000, 0x430007, K053246_word_w },
	{ 0x450010, 0x45001f, K053247_reg_word_w },
	{ 0x460000, 0x46001f, MWA16_RAM, &K053936_0_ctrl  },
	{ 0x470000, 0x470fff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x480000, 0x48003f, K056832_word_w },		/* VACSET*/
	{ 0x482000, 0x482007, K056832_b_word_w },	/* VSCCS*/
	{ 0x484000, 0x484003, ddd_053936_clip_w },
	{ 0x486000, 0x48601f, K053252_word_w },
	{ 0x488000, 0x4880ff, K055555_word_w },
	{ 0x48a00c, 0x48a00d, sound_cmd1_msb_w },
	{ 0x48a00e, 0x48a00f, sound_cmd2_msb_w },
	{ 0x48a000, 0x48a01f, MWA16_RAM },	/* sound regs fall-through*/
	{ 0x48c000, 0x48c01f, K054338_word_w },
	{ 0x600000, 0x60ffff, MWA16_RAM, &gx_workram },
	{ 0x660000, 0x6600ff, K054000_lsb_w },
	{ 0x6a0000, 0x6a0001, mmeeprom_w },
	{ 0x6c0000, 0x6c0001, ddd_053936_enable_w },
	{ 0x6e0000, 0x6e0001, sound_irq_w },
	{ 0xe00000, 0xe00001, MWA16_NOP }, 	/* watchdog*/
MEMORY_END

/**********************************************************************************/

static int cur_sound_region;

static void reset_sound_region(void)
{
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + cur_sound_region*0x4000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = (data & 0xf);
	reset_sound_region();
}

static INTERRUPT_GEN(audio_interrupt)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

/* sound memory maps

   there are 2 sound boards: the martial champion single-'539 version
   and the dual-'539 version used by run and gun, violent storm, monster maulers,
   gaiapolous, metamorphic force, and mystic warriors.  Their memory maps are
   quite similar to xexex/gijoe/asterix's sound.
 */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe230, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe62f, K054539_1_r },
	{ 0xe630, 0xe7ff, MRA_RAM },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_NOP },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, sound_bankswitch_w },
	{ 0xfff0, 0xfff3, MWA_NOP },	/* unknown write*/
MEMORY_END

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};

/**********************************************************************************/

static struct GfxLayout bglayout_4bpp =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*64
};

static struct GfxLayout bglayout_8bpp =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*128
};

static struct GfxDecodeInfo gfxdecodeinfo_gaiapols[] =
{
	{ REGION_GFX3, 0, &bglayout_4bpp, 0x0000, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_dadandrn[] =
{
	{ REGION_GFX3, 0, &bglayout_8bpp, 0x0000, 8 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( mystwarr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)	/* 16 MHz (confirmed) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(mystwarr_interrupt, 3)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 480)

	MDRV_INTERLEAVE(32);
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(600)

	MDRV_NVRAM_HANDLER(mystwarr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_RGB_DIRECT | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(24, 24+288-1, 16, 16+224-1)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(mystwarr)
	MDRV_VIDEO_UPDATE(mystwarr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("539", K054539, k054539_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( viostorm )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(vsreadmem,vswritemem)
	MDRV_CPU_VBLANK_INT(metamrph_interrupt, 40)
	MDRV_VBLANK_DURATION(900)

	/* video hardware */
	MDRV_VIDEO_START(viostorm)
	MDRV_VIDEO_UPDATE(metamrph)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(40, 40+384-1, 16, 16+224-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( metamrph )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mmreadmem,mmwritemem)
	MDRV_CPU_VBLANK_INT(metamrph_interrupt, 40)
	MDRV_VBLANK_DURATION(900)

	/* video hardware */
	MDRV_VIDEO_START(metamrph)
	MDRV_VIDEO_UPDATE(metamrph)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(24, 24+288-1, 17, 17+224-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dadandrn )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(dddreadmem, dddwritemem)
	MDRV_CPU_VBLANK_INT(ddd_interrupt, 1)
	MDRV_VBLANK_DURATION(600)

	MDRV_GFXDECODE(gfxdecodeinfo_dadandrn)

	/* video hardware */
	MDRV_VIDEO_START(dadandrn)
	MDRV_VIDEO_UPDATE(dadandrn)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(24, 24+288-1, 17, 17+224-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gaiapols )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(gaiareadmem,gaiawritemem)
	MDRV_CPU_VBLANK_INT(ddd_interrupt, 1)
	MDRV_VBLANK_DURATION(600)

	MDRV_GFXDECODE(gfxdecodeinfo_gaiapols)

	MDRV_NVRAM_HANDLER(gaiapols)

	/* video hardware */
	MDRV_VIDEO_START(gaiapols)
	MDRV_VIDEO_UPDATE(dadandrn)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(40, 40+376-1, 16, 16+224-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( martchmp )
	MDRV_IMPORT_FROM(mystwarr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mcreadmem,mcwritemem)
	MDRV_CPU_VBLANK_INT(mchamp_interrupt, 2)
	MDRV_VBLANK_DURATION(0)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_RGB_DIRECT | VIDEO_UPDATE_BEFORE_VBLANK)

	MDRV_VIDEO_START(martchmp)
	MDRV_VIDEO_UPDATE(martchmp)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(32, 32+384-1, 16, 16+224-1)
MACHINE_DRIVER_END

/**********************************************************************************/

INPUT_PORTS_START( mystwarr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* game loops if this is set */
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x40, 0x00, "Number of Players")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPSETTING(    0x40, "2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( metamrph )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism")
	PORT_DIPSETTING(    0x20, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x40, 0x00, "Number of Players")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPSETTING(    0x40, "2")
	PORT_DIPNAME( 0x80, 0x80, "Continuous Energy Increment")
	PORT_DIPSETTING(    0x80, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( viostorm )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Coin Mechanism")
	PORT_DIPSETTING(    0x40, "Common")
	PORT_DIPSETTING(    0x00, "Independant")
	PORT_DIPNAME( 0x80, 0x00, "Number of Players")
	PORT_DIPSETTING(    0x00, "3")
	PORT_DIPSETTING(    0x80, "2")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( dadandrn )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, "Background Detail" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x01, "Med" )
	PORT_DIPSETTING(    0x02, "High" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, "Character Detail" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x01, "Med" )
	PORT_DIPSETTING(    0x02, "High" )
INPUT_PORTS_END

INPUT_PORTS_START( martchmp )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* game loops if this is set */
	PORT_DIPNAME( 0x10, 0x00, "Sound Output")
	PORT_DIPSETTING(    0x10, "Mono")
	PORT_DIPSETTING(    0x00, "Stereo")
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

/**********************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

#define ROM_LOADTILE_WORD(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(3) | ROM_REVERSE)
#define ROM_LOADTILE_BYTE(name,offset,length,crc) ROMX_LOAD(name, offset, length, crc, ROM_GROUPBYTE | ROM_SKIP(4))

ROM_START( mystwarr )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "128eaa01.20f", 0x000000, 0x40000, CRC(508f249c) SHA1(d27a272ec63e4a39379c4a23fe15c4fb45674cb4) )
	ROM_LOAD16_BYTE( "128eaa02.20g", 0x000001, 0x40000, CRC(f8ffa352) SHA1(678c59d5fbb85d808e842947621b7f13669d35b5) )
	ROM_LOAD16_BYTE( "128a03.19f", 0x100000, 0x80000, CRC(e98094f3) SHA1(a3f9b804ff487f792a00ce85a383868ab0b1b5d8) )
	ROM_LOAD16_BYTE( "128a04.19g", 0x100001, 0x80000, CRC(88c6a3e4) SHA1(7c2361f716a2320730a3dd6723a271e349ad61c3) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("128a05.6b", 0x000000, 0x020000, CRC(0e5194e0) SHA1(83356158d561f1b8e21f6ae5936b61da834a0545) )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "128a08.1h", 0x000000, 1*1024*1024, CRC(63d6cfa0) SHA1(324bf25cf79aa030d2dcc94a53c1984eb8abec3a) )
	ROM_LOADTILE_WORD( "128a09.1k", 0x000002, 1*1024*1024, CRC(573a7725) SHA1(f2fef32053ed2a65c6c3ddd3e1657a866aa80b3e) )
	ROM_LOADTILE_BYTE( "128a10.3h", 0x000004, 512*1024, CRC(558e545a) SHA1(cac53e545f3f8980d431443f2c3b8b95e6077d1c) )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "128a16.22k", 0x000000, 1*1024*1024, CRC(459b6407) SHA1(e4dace4912f9558bee75a8e95ee2637f5e950b47) )
	ROM_LOAD64_WORD( "128a15.20k", 0x000002, 1*1024*1024, CRC(6bbfedf4) SHA1(0b3acb2b34c722ddc60c0e64e12baa1f225e4fbb) )
	ROM_LOAD64_WORD( "128a14.19k", 0x000004, 1*1024*1024, CRC(f7bd89dd) SHA1(c9b2ebd5a49840f8b260d53c25cfcc238d21c75c) )
	ROM_LOAD64_WORD( "128a13.17k", 0x000006, 1*1024*1024, CRC(e89b66a2) SHA1(fce6e56d1759ffe987766426ecb28e9015a500b7) )
	ROM_LOAD16_BYTE( "128a12.12k", 0x400000, 512*1024, CRC(63de93e2) SHA1(c9a50e7beff1cbbc5d5820664adbd54d52782c54) )
	ROM_LOAD16_BYTE( "128a11.10k", 0x400001, 512*1024, CRC(4eac941a) SHA1(c0a33f4b975ebee217fd335001839992f4c0bdc8) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "128a06.2d", 0x000000, 2*1024*1024, CRC(88ed598c) SHA1(3c123e26b3a12541df77b368bc0e0d486f5622b6) )
	ROM_LOAD( "128a07.1d", 0x200000, 2*1024*1024, CRC(db79a66e) SHA1(b7e118ed26bac557038e8ae6cb77f23f3da5646f) )
ROM_END

ROM_START( mystwaru )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "128uaa01.20f", 0x000000, 0x40000, CRC(3a89aafd) SHA1(6d2ebb7e04d262545276c8dbe1c63405e5de4901) )
	ROM_LOAD16_BYTE( "128uaa02.20g", 0x000001, 0x40000, CRC(de07410f) SHA1(4583cb4402b3b046f185fa6d5a1cfaa8fe0d858c) )
	ROM_LOAD16_BYTE( "128a03.19f", 0x100000, 0x80000, CRC(e98094f3) SHA1(a3f9b804ff487f792a00ce85a383868ab0b1b5d8) )
	ROM_LOAD16_BYTE( "128a04.19g", 0x100001, 0x80000, CRC(88c6a3e4) SHA1(7c2361f716a2320730a3dd6723a271e349ad61c3) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("128a05.6b", 0x000000, 0x020000, CRC(0e5194e0) SHA1(83356158d561f1b8e21f6ae5936b61da834a0545) )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "128a08.1h", 0x000000, 1*1024*1024, CRC(63d6cfa0) SHA1(324bf25cf79aa030d2dcc94a53c1984eb8abec3a) )
	ROM_LOADTILE_WORD( "128a09.1k", 0x000002, 1*1024*1024, CRC(573a7725) SHA1(f2fef32053ed2a65c6c3ddd3e1657a866aa80b3e) )
	ROM_LOADTILE_BYTE( "128a10.3h", 0x000004, 512*1024, CRC(558e545a) SHA1(cac53e545f3f8980d431443f2c3b8b95e6077d1c) )

	/* sprites */
	ROM_REGION( 0x500000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "128a16.22k", 0x000000, 1*1024*1024, CRC(459b6407) SHA1(e4dace4912f9558bee75a8e95ee2637f5e950b47) )
	ROM_LOAD64_WORD( "128a15.20k", 0x000002, 1*1024*1024, CRC(6bbfedf4) SHA1(0b3acb2b34c722ddc60c0e64e12baa1f225e4fbb) )
	ROM_LOAD64_WORD( "128a14.19k", 0x000004, 1*1024*1024, CRC(f7bd89dd) SHA1(c9b2ebd5a49840f8b260d53c25cfcc238d21c75c) )
	ROM_LOAD64_WORD( "128a13.17k", 0x000006, 1*1024*1024, CRC(e89b66a2) SHA1(fce6e56d1759ffe987766426ecb28e9015a500b7) )
	ROM_LOAD16_BYTE( "128a12.12k", 0x400000, 512*1024, CRC(63de93e2) SHA1(c9a50e7beff1cbbc5d5820664adbd54d52782c54) )
	ROM_LOAD16_BYTE( "128a11.10k", 0x400001, 512*1024, CRC(4eac941a) SHA1(c0a33f4b975ebee217fd335001839992f4c0bdc8) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "128a06.2d", 0x000000, 2*1024*1024, CRC(88ed598c) SHA1(3c123e26b3a12541df77b368bc0e0d486f5622b6) )
	ROM_LOAD( "128a07.1d", 0x200000, 2*1024*1024, CRC(db79a66e) SHA1(b7e118ed26bac557038e8ae6cb77f23f3da5646f) )
ROM_END

ROM_START( viostorm )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "168eab01.15h", 0x000001, 0x80000, CRC(4eee6a8e) SHA1(5c83ed2011aa77f590abca4c469fdb565f35dde5) )
	ROM_LOAD16_BYTE( "168eab02.15f", 0x000000, 0x80000, CRC(8dd8aa4c) SHA1(e7937fe1272b635807ffff08a45a0338d48c376c) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, CRC(507fb3eb) SHA1(a4f676e3caaafe86918c76ded08d0c202969adf6) )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, CRC(1b34a881) SHA1(5de20f7ee7f90d4f6dea349ca5000bfcf74253b1) )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, CRC(db0ce743) SHA1(dfe24a1e3e72da188a92668928e79afd6c5d22ee) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, CRC(bd2bbdea) SHA1(54faf2ded16e66d675bbbec4ebd42b4708edfaef) )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, CRC(7a57c9e7) SHA1(8763c310f7b515aef52d4e007bc949e8803690f4) )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, CRC(b6b1c4ef) SHA1(064ab4db884c8f98ab9e631b7034996d4b92ab7b) )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, CRC(cdec3650) SHA1(949bc06bb38a2d5315ee4f6db19e043655b90e6e) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, CRC(25404fd7) SHA1(282cf523728b38d0bf14d765dd7257aa1fb2af39) )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, CRC(fdbbf8cc) SHA1(a8adf72a25fe2b9c4c338350d02c92deb5f8c8e9) )
ROM_END

ROM_START( viostrmu )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "168uab01.15h", 0x000001, 0x80000, CRC(2d6a9fa3) SHA1(a2f82702896eddb11cd2b2f9ed5fff730f6baf0f) )
	ROM_LOAD16_BYTE( "168uab02.15f", 0x000000, 0x80000, CRC(0e75f7cc) SHA1(57af86703dc728ba83ca12889246c93b9f8d4576) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, CRC(507fb3eb) SHA1(a4f676e3caaafe86918c76ded08d0c202969adf6) )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00)
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, CRC(1b34a881) SHA1(5de20f7ee7f90d4f6dea349ca5000bfcf74253b1) )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, CRC(db0ce743) SHA1(dfe24a1e3e72da188a92668928e79afd6c5d22ee) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00)
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, CRC(bd2bbdea) SHA1(54faf2ded16e66d675bbbec4ebd42b4708edfaef) )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, CRC(7a57c9e7) SHA1(8763c310f7b515aef52d4e007bc949e8803690f4) )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, CRC(b6b1c4ef) SHA1(064ab4db884c8f98ab9e631b7034996d4b92ab7b) )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, CRC(cdec3650) SHA1(949bc06bb38a2d5315ee4f6db19e043655b90e6e) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0)

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, CRC(25404fd7) SHA1(282cf523728b38d0bf14d765dd7257aa1fb2af39) )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, CRC(fdbbf8cc) SHA1(a8adf72a25fe2b9c4c338350d02c92deb5f8c8e9) )
ROM_END

ROM_START( viostrma )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "168aac01.15h", 0x000001, 0x80000, CRC(3620635c) SHA1(d296ba707a131bd78b401608d6b165b214f4fe61) )
	ROM_LOAD16_BYTE( "168aac02.15f", 0x000000, 0x80000, CRC(db679aec) SHA1(233f3ab54125db1035cb0afadb06312ef7bd3e09) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, CRC(507fb3eb) SHA1(a4f676e3caaafe86918c76ded08d0c202969adf6) )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, CRC(1b34a881) SHA1(5de20f7ee7f90d4f6dea349ca5000bfcf74253b1) )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, CRC(db0ce743) SHA1(dfe24a1e3e72da188a92668928e79afd6c5d22ee) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, CRC(bd2bbdea) SHA1(54faf2ded16e66d675bbbec4ebd42b4708edfaef) )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, CRC(7a57c9e7) SHA1(8763c310f7b515aef52d4e007bc949e8803690f4) )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, CRC(b6b1c4ef) SHA1(064ab4db884c8f98ab9e631b7034996d4b92ab7b) )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, CRC(cdec3650) SHA1(949bc06bb38a2d5315ee4f6db19e043655b90e6e) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, CRC(25404fd7) SHA1(282cf523728b38d0bf14d765dd7257aa1fb2af39) )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, CRC(fdbbf8cc) SHA1(a8adf72a25fe2b9c4c338350d02c92deb5f8c8e9) )
ROM_END

ROM_START( viostrmj )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "168jac01.b01", 0x000001, 0x80000, CRC(f8be1225) SHA1(8c38ca218c0005c60a48cd3a43b5460b63a851e7) )
	ROM_LOAD16_BYTE( "168jac02.b02", 0x000000, 0x80000, CRC(f42fd1e5) SHA1(3b17c3039d800487f6117595050e7896a413db04) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("168a05.7c", 0x000000, 0x020000, CRC(507fb3eb) SHA1(a4f676e3caaafe86918c76ded08d0c202969adf6) )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "168a09.1h", 0x000000, 2*1024*1024, CRC(1b34a881) SHA1(5de20f7ee7f90d4f6dea349ca5000bfcf74253b1) )
	ROM_LOADTILE_WORD( "168a08.1k", 0x000002, 2*1024*1024, CRC(db0ce743) SHA1(dfe24a1e3e72da188a92668928e79afd6c5d22ee) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "168a10.22k", 0x000000, 2*1024*1024, CRC(bd2bbdea) SHA1(54faf2ded16e66d675bbbec4ebd42b4708edfaef) )
	ROM_LOAD64_WORD( "168a11.19k", 0x000002, 2*1024*1024, CRC(7a57c9e7) SHA1(8763c310f7b515aef52d4e007bc949e8803690f4) )
	ROM_LOAD64_WORD( "168a12.20k", 0x000004, 2*1024*1024, CRC(b6b1c4ef) SHA1(064ab4db884c8f98ab9e631b7034996d4b92ab7b) )
	ROM_LOAD64_WORD( "168a13.17k", 0x000006, 2*1024*1024, CRC(cdec3650) SHA1(949bc06bb38a2d5315ee4f6db19e043655b90e6e) )

	/* road generator */
	ROM_REGION( 0x40000, REGION_GFX3, 0 )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "168a06.1c", 0x000000, 2*1024*1024, CRC(25404fd7) SHA1(282cf523728b38d0bf14d765dd7257aa1fb2af39) )
	ROM_LOAD( "168a07.1e", 0x200000, 2*1024*1024, CRC(fdbbf8cc) SHA1(a8adf72a25fe2b9c4c338350d02c92deb5f8c8e9) )
ROM_END

ROM_START( metamrph )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "224a01", 0x000001, 0x40000, CRC(e1d9b516) SHA1(387ed5ce87af376c0568e39187330a4585dc499a) )
	ROM_LOAD16_BYTE( "224a02", 0x000000, 0x40000, CRC(289c926b) SHA1(5a5129fd85afc4fee97155e28bff89d3bb497b59) )
	ROM_LOAD16_BYTE( "224a03", 0x100001, 0x80000, CRC(a5bedb01) SHA1(5e7a0b93af654ba6a87be8d449c7080a0f0e2a43) )
	ROM_LOAD16_BYTE( "224a04", 0x100000, 0x80000, CRC(ada53ba4) SHA1(f77bf854dff1f8f718579fe6d3730066708396e2) )

	/* sound program */
	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	ROM_LOAD("224a05", 0x000000, 0x040000, CRC(4b4c985c) SHA1(c83cce05355023be9cd55b4aa595c61f8236269c) )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "224a09", 0x000000, 1*1024*1024, CRC(1931afce) SHA1(78838c0fd2a9c80f130db1fcf6c88b14f7363639) )
	ROM_LOADTILE_WORD( "224a08", 0x000002, 1*1024*1024, CRC(dc94d53a) SHA1(91e16371a335f078a81c06a1045759653080aba0) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "224a10", 0x000000, 2*1024*1024, CRC(161287f0) SHA1(a13b197a98fa1cebb11fb87b54e277c72852c4ee) )
	ROM_LOAD64_WORD( "224a11", 0x000002, 2*1024*1024, CRC(df5960e1) SHA1(ee7794dd119f5f2c52e7ba589d78067a89ff3cab) )
	ROM_LOAD64_WORD( "224a12", 0x000004, 2*1024*1024, CRC(ca72a4b3) SHA1(a09deb6d7cb8be4edaeb78e0e676ea2d6055e9e0) )
	ROM_LOAD64_WORD( "224a13", 0x000006, 2*1024*1024, CRC(86b58feb) SHA1(5a43746e2cd3c7aca21496c092aef83e64b3ab2c) )

	/* K053250 linescroll/zoom thingy */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* NOTE: region must be 2xROM size for unpacking*/
	ROM_LOAD( "224a14", 0x000000, 0x40000, CRC(3c79b404) SHA1(7c6bb4cbf050f314ea0cd3e8bc6e1947d0573084) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "224a06", 0x000000, 2*1024*1024, CRC(972f6abe) SHA1(30907495fc49fe3424c092b074c1dc137aa14306) )
	ROM_LOAD( "224a07", 0x200000, 1*1024*1024, CRC(61b2f97a) SHA1(34bf835d6361c7809d40fa20fd238c9e2a84b101) )
ROM_END

ROM_START( metamrpj )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "224jaa01", 0x000001, 0x40000, CRC(558d2602) SHA1(2dbc16fcc07436ca7eff8d070196348f272b8723) )
	ROM_LOAD16_BYTE( "224jaa02", 0x000000, 0x40000, CRC(9b252ace) SHA1(efe8cd942f3d4e2366d9af0fb9647d2a4aeac2c9) )
	ROM_LOAD16_BYTE( "224a03", 0x100001, 0x80000, CRC(a5bedb01) SHA1(5e7a0b93af654ba6a87be8d449c7080a0f0e2a43) )
	ROM_LOAD16_BYTE( "224a04", 0x100000, 0x80000, CRC(ada53ba4) SHA1(f77bf854dff1f8f718579fe6d3730066708396e2) )

	/* sound program */
	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	ROM_LOAD("224a05", 0x000000, 0x040000, CRC(4b4c985c) SHA1(c83cce05355023be9cd55b4aa595c61f8236269c) )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "224a09", 0x000000, 1*1024*1024, CRC(1931afce) SHA1(78838c0fd2a9c80f130db1fcf6c88b14f7363639) )
	ROM_LOADTILE_WORD( "224a08", 0x000002, 1*1024*1024, CRC(dc94d53a) SHA1(91e16371a335f078a81c06a1045759653080aba0) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "224a10", 0x000000, 2*1024*1024, CRC(161287f0) SHA1(a13b197a98fa1cebb11fb87b54e277c72852c4ee) )
	ROM_LOAD64_WORD( "224a11", 0x000002, 2*1024*1024, CRC(df5960e1) SHA1(ee7794dd119f5f2c52e7ba589d78067a89ff3cab) )
	ROM_LOAD64_WORD( "224a12", 0x000004, 2*1024*1024, CRC(ca72a4b3) SHA1(a09deb6d7cb8be4edaeb78e0e676ea2d6055e9e0) )
	ROM_LOAD64_WORD( "224a13", 0x000006, 2*1024*1024, CRC(86b58feb) SHA1(5a43746e2cd3c7aca21496c092aef83e64b3ab2c) )

	/* K053250 linescroll/zoom thingy */
	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* NOTE: region must be 2xROM size for unpacking*/
	ROM_LOAD( "224a14", 0x000000, 0x40000, CRC(3c79b404) SHA1(7c6bb4cbf050f314ea0cd3e8bc6e1947d0573084) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "224a06", 0x000000, 2*1024*1024, CRC(972f6abe) SHA1(30907495fc49fe3424c092b074c1dc137aa14306) )
	ROM_LOAD( "224a07", 0x200000, 1*1024*1024, CRC(61b2f97a) SHA1(34bf835d6361c7809d40fa20fd238c9e2a84b101) )
ROM_END

ROM_START( mtlchamp )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
        ROM_LOAD16_BYTE( "234eaa01.20f", 0x000000, 0x040000, CRC(8fa731db) SHA1(e2ed81762070a764f51aae36ce03859b5d86520d) )
        ROM_LOAD16_BYTE( "234eaa02.20g", 0x000001, 0x040000, CRC(e7b50b54) SHA1(594391a37479e6973aadd1fc866671c306a75497) )
	ROM_LOAD16_BYTE( "234_d03.19f", 0x300000, 0x80000, CRC(abb577c6) SHA1(493f11a10a4d5b62d755ff8274e77d898544944f) )
	ROM_LOAD16_BYTE( "234_d04.19g", 0x300001, 0x80000, CRC(030a1925) SHA1(03783488950c9f27af5948e7b9f6a609c2df6e0b) )

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("234_d05.6b", 0x000000, 0x020000, CRC(efb6bcaa) SHA1(4fb24b89a50b341871945547859278a6e2f5e002) )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "234a08.1h", 0x000000, 1*1024*1024, CRC(27e94288) SHA1(a92b03adf7beea6a1ceb74f659c87c628a7ab8e4) )
	ROM_LOADTILE_WORD( "234a09.1k", 0x000002, 1*1024*1024, CRC(03aad28f) SHA1(e7d9d788822ac9666e089b58288e3fcdba1b89da) )
	ROM_LOADTILE_BYTE( "234a10.3h", 0x000004, 512*1024, CRC(51f50fe2) SHA1(164fc975feff442d93f1917727c159051dcd3a55) )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "234a16.22k", 0x000000, 2*1024*1024, CRC(14d909a5) SHA1(15da356852fc0c63ecd924ac37ebe24bf3ba0760) )
	ROM_LOAD64_WORD( "234a15.20k", 0x000002, 2*1024*1024, CRC(a5028418) SHA1(ec6fc7b38fb1d27490a5a9310ecac2d1049e197c) )
	ROM_LOAD64_WORD( "234a14.19k", 0x000004, 2*1024*1024, CRC(d7921f47) SHA1(3fc97b308ad2ca25a376373ddfe08c8a375c424e) )
	ROM_LOAD64_WORD( "234a13.17k", 0x000006, 2*1024*1024, CRC(5974392e) SHA1(7c380419244439804797a9510846d273ebe99d02) )
	ROM_LOAD16_BYTE( "234a12.12k", 0x800000, 1024*1024, CRC(c7f2b099) SHA1(b72b80feb52560a5a42a1db39b059ac8bca27c10) )
	ROM_LOAD16_BYTE( "234a11.10k", 0x800001, 1024*1024, CRC(82923713) SHA1(a36cd3b2c9d36e93a3c25ba1d4e162f3d92e06ae) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "234a06.2d", 0x000000, 2*1024*1024, CRC(12d32384) SHA1(ecd6cd752b0e20339e17a7652ed843fbb43f7595) )
	ROM_LOAD( "234a07.1d", 0x200000, 2*1024*1024, CRC(05ee239f) SHA1(f4e6e7568dc73666a2b5e0c3fe743432e0436464) )
ROM_END

ROM_START( mtlchmpj )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "234jaa01.20f", 0x000000, 0x40000, CRC(76c3c568) SHA1(14c0009804fcedd8a3e5a105cc871dd6cd4cf7b3) )
	ROM_LOAD16_BYTE( "234jaa02.20g", 0x000001, 0x40000, CRC(95eec0aa) SHA1(11f1986d792951d6d5b3740b435dab2a2f4e6cbd) )
	ROM_LOAD16_BYTE( "234_d03.19f", 0x300000, 0x80000, CRC(abb577c6) SHA1(493f11a10a4d5b62d755ff8274e77d898544944f) )
	ROM_LOAD16_BYTE( "234_d04.19g", 0x300001, 0x80000, CRC(030a1925) SHA1(03783488950c9f27af5948e7b9f6a609c2df6e0b) )

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("234_d05.6b", 0x000000, 0x020000, CRC(efb6bcaa) SHA1(4fb24b89a50b341871945547859278a6e2f5e002) )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "234a08.1h", 0x000000, 1*1024*1024, CRC(27e94288) SHA1(a92b03adf7beea6a1ceb74f659c87c628a7ab8e4) )
	ROM_LOADTILE_WORD( "234a09.1k", 0x000002, 1*1024*1024, CRC(03aad28f) SHA1(e7d9d788822ac9666e089b58288e3fcdba1b89da) )
	ROM_LOADTILE_BYTE( "234a10.3h", 0x000004, 512*1024, CRC(51f50fe2) SHA1(164fc975feff442d93f1917727c159051dcd3a55) )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "234a16.22k", 0x000000, 2*1024*1024, CRC(14d909a5) SHA1(15da356852fc0c63ecd924ac37ebe24bf3ba0760) )
	ROM_LOAD64_WORD( "234a15.20k", 0x000002, 2*1024*1024, CRC(a5028418) SHA1(ec6fc7b38fb1d27490a5a9310ecac2d1049e197c) )
	ROM_LOAD64_WORD( "234a14.19k", 0x000004, 2*1024*1024, CRC(d7921f47) SHA1(3fc97b308ad2ca25a376373ddfe08c8a375c424e) )
	ROM_LOAD64_WORD( "234a13.17k", 0x000006, 2*1024*1024, CRC(5974392e) SHA1(7c380419244439804797a9510846d273ebe99d02) )
	ROM_LOAD16_BYTE( "234a12.12k", 0x800000, 1024*1024, CRC(c7f2b099) SHA1(b72b80feb52560a5a42a1db39b059ac8bca27c10) )
	ROM_LOAD16_BYTE( "234a11.10k", 0x800001, 1024*1024, CRC(82923713) SHA1(a36cd3b2c9d36e93a3c25ba1d4e162f3d92e06ae) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "234a06.2d", 0x000000, 2*1024*1024, CRC(12d32384) SHA1(ecd6cd752b0e20339e17a7652ed843fbb43f7595) )
	ROM_LOAD( "234a07.1d", 0x200000, 2*1024*1024, CRC(05ee239f) SHA1(f4e6e7568dc73666a2b5e0c3fe743432e0436464) )
ROM_END

ROM_START( gaiapols )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "123e07.24m", 0x000000, 1*1024*1024, CRC(f1a1db0f) SHA1(1394b8a57493cbc8d5bf555d363ad844a2407d98) )
	ROM_LOAD16_BYTE( "123e09.19l", 0x000001, 1*1024*1024, CRC(4b3b57e7) SHA1(1415ddc1393a468705d7a37d054ab6b08d0eb205) )

	/* 68k data */
	ROM_LOAD16_BYTE( "123jaf11.19p", 0x200000, 256*1024, CRC(19919571) SHA1(e4fbbdd4003f18631e5723bb85a7fa60e57f2d2a) )
	ROM_LOAD16_BYTE( "123jaf12.17p", 0x200001, 256*1024, CRC(4246e595) SHA1(d5fe0b1dfe2a0c64b3e62820dea5094cc0f5bd12) )

	/* sound program */
	ROM_REGION( 0x050000, REGION_CPU2, 0 )
	ROM_LOAD("123e13.9c", 0x000000, 0x040000, CRC(e772f822) SHA1(2a5cdfc0aacad56cbef8bdbe8319e7ff4ab71eee) )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "123e16.2t", 0x000000, 1*1024*1024, CRC(a3238200) SHA1(9ea073f7611b5c0b328c707c97ef174634c303bc) )
	ROM_LOADTILE_WORD( "123e17.2x", 0x000002, 1*1024*1024, CRC(bd0b9fb9) SHA1(1714c19d2123b8fd4cfedb66d13bdcee6fc77576) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "123e19.34u", 0x000000, 2*1024*1024, CRC(219a7c26) SHA1(0f24873523f91379862e0d4317fc42b8a10d412e) )
	ROM_LOAD64_WORD( "123e21.34y", 0x000002, 2*1024*1024, CRC(1888947b) SHA1(41a046cbcd2406add2ff48cb82a1353d9ac5fa3c) )
	ROM_LOAD64_WORD( "123e18.36u", 0x000004, 2*1024*1024, CRC(3719b6d4) SHA1(25ef6e8c3c7f151f1cee063356bdd56c713356ea) )
	ROM_LOAD64_WORD( "123e20.36y", 0x000006, 2*1024*1024, CRC(490a6f64) SHA1(81b1acc74ce834399005a748eae50b3d633fa469) )

	/* K053536 roz tiles */
	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "123e04.32n", 0x000000, 0x080000, CRC(0d4d5b8b) SHA1(d3fb0c77ad46ee9b9c704be6f174258aa051aa71) )
	ROM_LOAD( "123e05.29n", 0x080000, 0x080000, CRC(7d123f3e) SHA1(f9752e96515dc965aae04e01dfa813fcc4cbccd6) )
	ROM_LOAD( "123e06.26n", 0x100000, 0x080000, CRC(fa50121e) SHA1(4596a9b0a6cc67f259182098d3976234b6ed8cb6) )

	/* K053936 map data */
	ROM_REGION( 0xa0000, REGION_GFX4, 0 )
	ROM_LOAD( "123e01.36j", 0x000000, 0x20000, CRC(9dbc9678) SHA1(4183eb833d0d1cd710fac32071df7ebcb7a9c812) )
	ROM_LOAD( "123e02.34j", 0x020000, 0x40000, CRC(b8e3f500) SHA1(254c665b7aa534990e899fe8f54c3f24e8126fba) )
	ROM_LOAD( "123e03.36m", 0x060000, 0x40000, CRC(fde4749f) SHA1(7f9c09d11dcb16d72046c7605570c3a29e279fa9) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "123e14.2g", 0x000000, 2*1024*1024, CRC(65dfd3ff) SHA1(57e13c05f420747c1c2010cc5340dd70e2c28971) )
	ROM_LOAD( "123e15.2m", 0x200000, 2*1024*1024, CRC(7017ff07) SHA1(37ecd54f2c757c5385305ab726d9f66aa1afd456) )
ROM_END

ROM_START( dadandrn )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "170jaa07.24m", 0x000000, 0x80000, CRC(6a55e828) SHA1(06be9a45fdddcc826a33ff8d6eb595e11b98a31f) )
	ROM_LOAD16_BYTE( "170jaa09.19l", 0x000001, 0x80000, CRC(9e821cd8) SHA1(51e9c3b0a187db62cfcdff23ecaf3205f368f4e0) )
	ROM_LOAD16_BYTE( "170a08.21m", 0x100000, 0x40000, CRC(03c59ba2) SHA1(041473fe5f9004bfb7ca767c2004154c27f726ff) )
	ROM_LOAD16_BYTE( "170a10.17l", 0x100001, 0x40000, CRC(8a340909) SHA1(3e2ef2642e792cdc38b3442df67377ed9e70d3ab) )

	/* sound program */
	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD("170a13.9c", 0x000000, 0x40000, CRC(2ebf4d1c) SHA1(33a3f4153dfdc46cc223d216a17ef9428c09129d) )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "170a16.2t", 0x000000, 1*1024*1024, CRC(41fee912) SHA1(73cf167ac9fc42cb8048a87b6c6d1c3c0ae3c2e2) )
	ROM_LOADTILE_WORD( "170a17.2x", 0x000002, 1*1024*1024, CRC(96957c91) SHA1(b12d356f8a015ec0984bdb86da9c569eb0c67880) )
	ROM_LOADTILE_BYTE( "170a24.5r", 0x000004, 512*1024, CRC(562ad4bd) SHA1(f55b29142ea39f090244f0945a56760bab25c7a7) )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "170a19.34u", 0x000000, 2*1024*1024, CRC(be835141) SHA1(b76e1da45bf602dd9eb30fb8b7181cea2e820c3d) )
	ROM_LOAD64_WORD( "170a21.34y", 0x000002, 2*1024*1024, CRC(bcb68136) SHA1(1d453f59d832b8ea99cf0a60a917edce5c1c90a0) )
	ROM_LOAD64_WORD( "170a18.36u", 0x000004, 2*1024*1024, CRC(e1e3c8d2) SHA1(2c94fcedd1dcef3d3332af358ae8a67dea507216) )
	ROM_LOAD64_WORD( "170a20.36y", 0x000006, 2*1024*1024, CRC(ccb4d88c) SHA1(064b4dab0ca6e5a1fa2fc2e9bbb19c7499830ee1) )
	ROM_LOAD16_BYTE( "170a23.29y", 0x800000, 1024*1024, CRC(6b5390e4) SHA1(0c5066bc86e782db4b64c2a604aed89ae99af005) )
	ROM_LOAD16_BYTE( "170a22.32y", 0x800001, 1024*1024, CRC(21628106) SHA1(1e025ff53caa5cbbf7695f8a77736d59f8a8af1b) )

	/* K053536 roz plane */
	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "170a04.33n", 0x000000, 0x80000, CRC(64b9a73b) SHA1(8b984bfd8bdf6d93ad223fca46a4f958a0edb2be) )
	ROM_LOAD( "170a05.30n", 0x080000, 0x80000, CRC(f2c101d0) SHA1(d80045c9a02db08ea6c851bdc12826862e11c381) )
	ROM_LOAD( "170a06.27n", 0x100000, 0x80000, CRC(b032e59b) SHA1(482300c683db20c2b2fc6e007b8f7e35373e3c00) )

	/* K053936 tilemap data */
	ROM_REGION( 0x80000, REGION_GFX4, 0 )
	ROM_LOAD( "170a02.34j", 0x000000, 0x40000, CRC(b040cebf) SHA1(4d1ba4ee60fd7caf678837ec6f4d68fcbce1ccf2) )
	ROM_LOAD( "170a03.36m", 0x040000, 0x40000, CRC(7fb412b2) SHA1(f603a8f0becf88e345f4b7a68cf018962a255a1e) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD("170a14.2g", 0x000000, 2*1024*1024, CRC(83317cda) SHA1(c5398c5959ef3ea73835e13db69660dd28c31486) )
	ROM_LOAD("170a15.2m", 0x200000, 2*1024*1024, CRC(d4113ae9) SHA1(e234d06f462e3db64455c384c2f42174f9ef9c6a) )
ROM_END

ROM_START( mmaulers )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "170eaa07.24m", 0x000000, 0x80000, CRC(5458bd93) SHA1(d27a29076be3c745e3efdb1c528b07bd5d8aff1c) )
	ROM_LOAD16_BYTE( "170eaa09.19l", 0x000001, 0x80000, CRC(99c95c7b) SHA1(7f22930c2fe21205ccd01b80566d6bc31fea34d2) )
	ROM_LOAD16_BYTE( "170a08.21m",   0x100000, 0x40000, CRC(03c59ba2) SHA1(041473fe5f9004bfb7ca767c2004154c27f726ff) )
	ROM_LOAD16_BYTE( "170a10.17l",   0x100001, 0x40000, CRC(8a340909) SHA1(3e2ef2642e792cdc38b3442df67377ed9e70d3ab) )

	/* sound program */
	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD("170a13.9c", 0x000000, 0x40000, CRC(2ebf4d1c) SHA1(33a3f4153dfdc46cc223d216a17ef9428c09129d) )
	ROM_RELOAD(           0x010000, 0x040000 )

	/* tiles */
	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOADTILE_WORD( "170a16.2t", 0x000000, 1*1024*1024, CRC(41fee912) SHA1(73cf167ac9fc42cb8048a87b6c6d1c3c0ae3c2e2) )
	ROM_LOADTILE_WORD( "170a17.2x", 0x000002, 1*1024*1024, CRC(96957c91) SHA1(b12d356f8a015ec0984bdb86da9c569eb0c67880) )
	ROM_LOADTILE_BYTE( "170a24.5r", 0x000004, 512*1024, CRC(562ad4bd) SHA1(f55b29142ea39f090244f0945a56760bab25c7a7) )

	/* sprites */
	ROM_REGION( 0xa00000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD64_WORD( "170a19.34u", 0x000000, 2*1024*1024, CRC(be835141) SHA1(b76e1da45bf602dd9eb30fb8b7181cea2e820c3d) )
	ROM_LOAD64_WORD( "170a21.34y", 0x000002, 2*1024*1024, CRC(bcb68136) SHA1(1d453f59d832b8ea99cf0a60a917edce5c1c90a0) )
	ROM_LOAD64_WORD( "170a18.36u", 0x000004, 2*1024*1024, CRC(e1e3c8d2) SHA1(2c94fcedd1dcef3d3332af358ae8a67dea507216) )
	ROM_LOAD64_WORD( "170a20.36y", 0x000006, 2*1024*1024, CRC(ccb4d88c) SHA1(064b4dab0ca6e5a1fa2fc2e9bbb19c7499830ee1) )
	ROM_LOAD16_BYTE( "170a23.29y", 0x800000, 1024*1024, CRC(6b5390e4) SHA1(0c5066bc86e782db4b64c2a604aed89ae99af005) )
	ROM_LOAD16_BYTE( "170a22.32y", 0x800001, 1024*1024, CRC(21628106) SHA1(1e025ff53caa5cbbf7695f8a77736d59f8a8af1b) )

	/* K053536 roz plane */
	ROM_REGION( 0x180000, REGION_GFX3, 0 )
	ROM_LOAD( "170a04.33n", 0x000000, 0x80000, CRC(64b9a73b) SHA1(8b984bfd8bdf6d93ad223fca46a4f958a0edb2be) )
	ROM_LOAD( "170a05.30n", 0x080000, 0x80000, CRC(f2c101d0) SHA1(d80045c9a02db08ea6c851bdc12826862e11c381) )
	ROM_LOAD( "170a06.27n", 0x100000, 0x80000, CRC(b032e59b) SHA1(482300c683db20c2b2fc6e007b8f7e35373e3c00) )

	/* K053936 tilemap data */
	ROM_REGION( 0x80000, REGION_GFX4, 0 )
	ROM_LOAD( "170a02.34j", 0x000000, 0x40000, CRC(b040cebf) SHA1(4d1ba4ee60fd7caf678837ec6f4d68fcbce1ccf2) )
	ROM_LOAD( "170a03.36m", 0x040000, 0x40000, CRC(7fb412b2) SHA1(f603a8f0becf88e345f4b7a68cf018962a255a1e) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD("170a14.2g", 0x000000, 2*1024*1024, CRC(83317cda) SHA1(c5398c5959ef3ea73835e13db69660dd28c31486) )
	ROM_LOAD("170a15.2m", 0x200000, 2*1024*1024, CRC(d4113ae9) SHA1(e234d06f462e3db64455c384c2f42174f9ef9c6a) )
ROM_END

static void init_common(void)
{
	unsigned char *s = memory_region(REGION_GFX1);
	unsigned char *pFinish = s+memory_region_length(REGION_GFX1)-3;

	while( s<pFinish )
	{
		/* convert the whole mess to 5bpp planar in System GX's format
		   (p3 p1 p2 p0 p5)
		   (the original ROMs are stored as chunky for the first 4 bits
		   and the 5th bit is planar, which is undecodable as-is) */
		int d0 = ((s[0]&0x80)   )|((s[0]&0x08)<<3)|((s[1]&0x80)>>2)|((s[1]&0x08)<<1)|
		         ((s[2]&0x80)>>4)|((s[2]&0x08)>>1)|((s[3]&0x80)>>6)|((s[3]&0x08)>>3);
		int d1 = ((s[0]&0x40)<<1)|((s[0]&0x04)<<4)|((s[1]&0x40)>>1)|((s[1]&0x04)<<2)|
		         ((s[2]&0x40)>>3)|((s[2]&0x04)   )|((s[3]&0x40)>>5)|((s[3]&0x04)>>2);
		int d2 = ((s[0]&0x20)<<2)|((s[0]&0x02)<<5)|((s[1]&0x20)   )|((s[1]&0x02)<<3)|
		         ((s[2]&0x20)>>2)|((s[2]&0x02)<<1)|((s[3]&0x20)>>4)|((s[3]&0x02)>>1);
		int d3 = ((s[0]&0x10)<<3)|((s[0]&0x01)<<6)|((s[1]&0x10)<<1)|((s[1]&0x01)<<4)|
		         ((s[2]&0x10)>>1)|((s[2]&0x01)<<2)|((s[3]&0x10)>>3)|((s[3]&0x01)   );

		s[0] = d3;
		s[1] = d1;
		s[2] = d2;
		s[3] = d0;

		s += 5;
	}

	/* set default bankswitch */
	cur_sound_region = 2;
	reset_sound_region();

	mw_irq_control = 0;
	resume_trigger = 0;
	suspension_active = 0;

	state_save_register_int("Mystwarr", 0, "IRQ control", &mw_irq_control);
	state_save_register_int("Mystwarr", 0, "sound region", &cur_sound_region);
	state_save_register_func_postload(reset_sound_region);
}

static DRIVER_INIT(mystwarr)
{
	int i;

	init_common();

	#if MW_SKIPIDLE
		ADD_SKIPPER16(0x1bac, 0x200000, 0x540, 0x6ff, -1, 0xffff)
	#endif

	/* soften chorus(chip 0 channel 0-3), boost voice(chip 0 channel 4-7)*/
	for (i=0; i<=3; i++)
	{
		K054539_set_gain(0, i, 0.8);
		K054539_set_gain(0, i+4, 2.0);
	}

	/* soften percussions(chip 1 channel 0-7)*/
	for (i=0; i<=7; i++) K054539_set_gain(1, i, 0.5);
}

static DRIVER_INIT(dadandrn)
{
	int i;

	init_common();

	#if MW_SKIPIDLE
		ADD_SKIPPER16(0x442a, 0x600000, 0x400, 0x4ff, -1, 0xffff)
	#endif

	/* boost voice(chip 0 channel 4-7)*/
	for (i=4; i<=7; i++) K054539_set_gain(0, i, 2.0);
}

static DRIVER_INIT(viostorm)
{
	int i;

	init_common();

	#if MW_SKIPIDLE
		ADD_SKIPPER16(0x0a9c, 0x200000, 0xf400, 0xf9df, -1, 0xffff)
	#endif

	/* boost voice(chip 0 channel 4-7)*/
	for (i=4; i<=7; i++) K054539_set_gain(0, i, 2.0);
}

static DRIVER_INIT(metamrph)
{
	int i;

	init_common();

	K053250_unpack_pixels(REGION_GFX3);

	/* boost voice(chip 0 channel 4-7) and soften other channels*/
	for (i=0; i<=3; i++)
	{
		K054539_set_gain(0, i,   0.8);
		K054539_set_gain(0, i+4, 1.8);
		K054539_set_gain(1, i,   0.8);
		K054539_set_gain(1, i+4, 0.8);
	}
}

static DRIVER_INIT(martchmp)
{
	int i;

	init_common();

	K054539_init_flags(K054539_REVERSE_STEREO);

	/* boost voice(chip 0 channel 4-7)*/
	for (i=4; i<=7; i++) K054539_set_gain(0, i, 1.4);
}

static DRIVER_INIT(gaiapols)
{
	int i;

	init_common();

	#if MW_SKIPIDLE
		ADD_SKIPPER16(0x200e4a, 0x600000, 0x540, 0x6ff, -1, 0xffff)
	#endif

	/* boost voice(chip 0 channel 5-7)*/
	for (i=5; i<=7; i++) K054539_set_gain(0, i, 2.0);
}


/*           ROM       parent    machine   inp       init */
GAMEX ( 1993, mystwarr, 0,        mystwarr, mystwarr, mystwarr, ROT0,  "Konami", "Mystic Warriors (Europe ver EAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX ( 1993, mystwaru, mystwarr, mystwarr, mystwarr, mystwarr, ROT0,  "Konami", "Mystic Warriors (US ver UAA)", GAME_IMPERFECT_GRAPHICS )
GAMECX( 1993, mmaulers, 0,        dadandrn, dadandrn, dadandrn, ROT0,  "Konami", "Monster Maulers (ver EAA)", GAME_IMPERFECT_GRAPHICS, &generic_ctrl, &mmaulers_bootstrap )
GAMEX ( 1993, dadandrn, mmaulers, dadandrn, dadandrn, dadandrn, ROT0,  "Konami", "Kyukyoku Sentai Dadandarn (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMECX( 1993, viostorm, 0,        viostorm, viostorm, viostorm, ROT0,  "Konami", "Violent Storm (Europe ver EAB)", GAME_IMPERFECT_GRAPHICS, &generic_ctrl, &viostorm_bootstrap )
GAMEX ( 1993, viostrmu, viostorm, viostorm, viostorm, viostorm, ROT0,  "Konami", "Violent Storm (US ver UAB)", GAME_IMPERFECT_GRAPHICS )
GAMECX( 1993, viostrmj, viostorm, viostorm, viostorm, viostorm, ROT0,  "Konami", "Violent Storm (Japan ver JAC)", GAME_IMPERFECT_GRAPHICS, &generic_ctrl, &viostrmj_bootstrap )
GAMECX( 1993, viostrma, viostorm, viostorm, viostorm, viostorm, ROT0,  "Konami", "Violent Storm (Asia ver AAC)", GAME_IMPERFECT_GRAPHICS, &generic_ctrl, &viostrma_bootstrap )
GAMEX ( 1993, metamrph, 0,        metamrph, metamrph, metamrph, ROT0,  "Konami", "Metamorphic Force (US ver UAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX ( 1993, metamrpj, metamrph, metamrph, metamrph, metamrph, ROT0,  "Konami", "Metamorphic Force (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX ( 1993, mtlchamp, 0,        martchmp, martchmp, martchmp, ROT0,  "Konami", "Martial Champion (Europe ver EAA)", GAME_IMPERFECT_GRAPHICS )
GAMEX ( 1993, mtlchmpj, mtlchamp, martchmp, martchmp, martchmp, ROT0,  "Konami", "Martial Champion (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS )
GAMECX( 1993, gaiapols, 0,        gaiapols, dadandrn, gaiapols, ROT90, "Konami", "Gaiapolis (Japan ver JAF)", GAME_IMPERFECT_GRAPHICS, &generic_ctrl, &gaiapols_bootstrap )
