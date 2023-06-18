/*----------------------------------------------------------------
   Psikyo PS4 SH-2 Based Systems
   driver by David Haywood (+ Paul Priest)
------------------------------------------------------------------

Moving on from the 68020 based system used for the first Strikers
1945 game Psikyo introduced a system using Hitachi's SH-2 CPU

This driver is for the dual-screen PS4 boards

 Board PS4 (Custom Chip PS6807)
 ------------------------------
 Taisen Hot Gimmick (c)1997
 Taisen Hot Gimmick Kairakuten (c)1998
 Taisen Hot Gimmick 3 Digital Surfing (c)1999
 Taisen Hot Gimmick 4 Ever (c)2000 (confirmed by Japump)
 Lode Runner - The Dig Fight (c)2000
 Quiz de Idol! Hot Debut (c)2000

 The PS4 board appears to be a cheaper board than PS3/5/5v2, with only simple sprites, no bgs,
 smaller palette etc, only 8bpp sprites too.
 Supports dual-screen though.

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278B-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

  Sprite List format not 100% understood.
  Some of the games suffer from sound problems (wrong pitch / samples played)


*-----------------------------------*
|         Tips and Tricks           |
*-----------------------------------*

Hold Button during booting to test roms (Checksum 16-bit) for:

Lode Runner - The Dig Fight:   PL1 Start (passes gfx, sample result:05A5, expects:0BB0 [both sets])
Quiz de Idol! Hot Debut:       PL1 Start (passes)

--- Lode Runner: The Dig Fight ---

5-0-8-2-0 Maintenance Mode

--- Quiz de Idol! Hot Debut ---

9-2-3-0-1 Maintenance Mode

NOTE: The version number (A/B) on Lode Runner: The Dig Fight is ONLY displayed when the game is set
      to Japanese.  The same is true for Space Bomber in psikyosh.c

----------------------------------------------------------------*/

#include "driver.h"
#include "state.h"
#include "cpuintrf.h"

#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"

#define DUAL_SCREEN 0 /* Display both screens simultaneously if 1, change in vidhrdw too */
#define ROMTEST 0 /* Does necessary stuff to perform rom test, uses RAM as it doesn't dispose of GFX after decoding */

data32_t *psikyo4_vidregs, *ps4_ram, *ps4_io_select;
data32_t *bgpen_1, *bgpen_2, *screen1_brt, *screen2_brt;

#define MASTER_CLOCK 57272700	/* main oscillator frequency*/

/* defined in vidhrdw/psikyo4.c */
VIDEO_START( psikyo4 );
VIDEO_UPDATE( psikyo4 );

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfops4[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0x80 }, /* 8bpp tiles*/
	{ -1 }
};

static struct EEPROM_interface eeprom_interface_93C56 =
{
	8,		/* address bits	8*/
	8,		/* data bits	8*/
	"*110x",	/* read			110x aaaaaaaa*/
	"*101x",	/* write		101x aaaaaaaa dddddddd*/
	"*111x",	/* erase		111x aaaaaaaa*/
	"*10000xxxxxxx",/* lock			100x 00xxxx*/
	"*10011xxxxxxx",/* unlock		100x 11xxxx*/
/*	"*10001xxxx",	*/ /* write all	1 00 01xxxx dddddddddddddddd*/
/*	"*10010xxxx"	*/ /* erase all	1 00 10xxxx*/
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C56);
		if (file)
		{
			EEPROM_load(file);
		}
		else	/* these games want the eeprom all zeros by default*/
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);
		}
	}
}

static WRITE32_HANDLER( ps4_eeprom_w )
{
	if (ACCESSING_MSW32)
	{
		EEPROM_write_bit((data & 0x00200000) ? 1 : 0);
		EEPROM_set_cs_line((data & 0x00800000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);

		return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk EEPROM write %x mask %x\n", data, mem_mask);
}

static READ32_HANDLER( ps4_eeprom_r )
{
	if (ACCESSING_MSW32)
	{
		return ((EEPROM_read_bit() << 20)); /* EEPROM */
	}

	/* log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk EEPROM read mask %x\n", mem_mask); */

	return readinputport(10)<<16;
	
}

static INTERRUPT_GEN(psikyosh_interrupt)
{
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

static READ32_HANDLER(hotgmck_io32_r) /* used by hotgmck/hgkairak */
{
	int ret = 0xff;
	int sel = (ps4_io_select[0] & 0x0000ff00) >> 8;

	if (sel & 1) ret &= readinputport(0+4*offset);
	if (sel & 2) ret &= readinputport(1+4*offset);
	if (sel & 4) ret &= readinputport(2+4*offset);
	if (sel & 8) ret &= readinputport(3+4*offset);

	return ret<<24 | readinputport(8);
}

static READ32_HANDLER(ps4_io32_r) /* used by loderndf/hotdebut */
{
	return ((readinputport(0+4*offset) << 24) | (readinputport(1+4*offset) << 16) | (readinputport(2+4*offset) << 8) | (readinputport(3+4*offset) << 0));
}

static WRITE32_HANDLER( ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
	palette_set_color(offset+0x800,r,g,b); /* For screen 2*/
}

static WRITE32_HANDLER( ps4_bgpen_1_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_1[0]);

	b = ((bgpen_1[0] & 0x0000ff00) >>8);
	g = ((bgpen_1[0] & 0x00ff0000) >>16);
	r = ((bgpen_1[0] & 0xff000000) >>24);

	palette_set_color(0x1000,r,g,b); /* Clear colour for screen 1*/
}

static WRITE32_HANDLER( ps4_bgpen_2_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_2[0]);

	b = ((bgpen_2[0] & 0x0000ff00) >>8);
	g = ((bgpen_2[0] & 0x00ff0000) >>16);
	r = ((bgpen_2[0] & 0xff000000) >>24);

	palette_set_color(0x1001,r,g,b); /* Clear colour for screen 2*/
}

static WRITE32_HANDLER( ps4_screen1_brt_w )
{
	if(ACCESSING_LSB32) {
		/* Need seperate brightness for both screens if displaying together */
		double brt1 = data & 0xff;
		static double oldbrt1;

		if (brt1>0x7f) brt1 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt1 = (0x7f - brt1) / 127.0;
		if (oldbrt1 != brt1)
		{
			int i;

			for (i = 0; i < 0x800; i++)
				palette_set_brightness(i,brt1);

			oldbrt1 = brt1;
		}
	} else {
		/* I believe this to be seperate rgb brightness due to strings in hotdebut, unused in 4 dumped games */
		if((data & ~mem_mask) != 0)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk Scr 1 rgb? brt write %08x mask %08x\n", data, mem_mask);
	}
}

static WRITE32_HANDLER( ps4_screen2_brt_w )
{
	if(ACCESSING_LSB32) {
		/* Need seperate brightness for both screens if displaying together */
		double brt2 = data & 0xff;
		static double oldbrt2;

		if (brt2>0x7f) brt2 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt2 = (0x7f - brt2) / 127.0;

		if (oldbrt2 != brt2)
		{
			int i;

			for (i = 0x800; i < 0x1000; i++)
				palette_set_brightness(i,brt2);

			oldbrt2 = brt2;
		}
	} else {
		/* I believe this to be seperate rgb brightness due to strings in hotdebut, unused in 4 dumped games */
		if((data & ~mem_mask) != 0)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk Scr 2 rgb? brt write %08x mask %08x\n", data, mem_mask);
	}
}

static WRITE32_HANDLER( ps4_vidregs_w )
{
	COMBINE_DATA(&psikyo4_vidregs[offset]);

#if ROMTEST
	if(offset==2) /* Configure bank for gfx test */
	{
		if (!(mem_mask & 0x000000ff) || !(mem_mask & 0x0000ff00))	/* Bank*/
		{
			unsigned char *ROM = memory_region(REGION_GFX1);
			cpu_setbank(2,&ROM[0x2000 * (psikyo4_vidregs[offset]&0xfff)]); /* Bank comes from vidregs */
		}
	}
#endif
}

#if ROMTEST
static UINT32 sample_offs = 0;

static READ32_HANDLER( ps4_sample_r ) /* Send sample data for test */
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	return ROM[sample_offs++]<<16;
}
#endif

static READ32_HANDLER( psh_ymf_fm_r )
{
	return YMF278B_status_port_0_r(0)<<24; /* Also, bit 0 being high indicates not ready to send sample data for test */
}

static WRITE32_HANDLER( psh_ymf_fm_w )
{
	if (!(mem_mask & 0xff000000))	/* FM bank 1 address (OPL2/OPL3 compatible)*/
	{
		YMF278B_control_port_0_A_w(0, data>>24);
	}

	if (!(mem_mask & 0x00ff0000))	/* FM bank 1 data*/
	{
		YMF278B_data_port_0_A_w(0, data>>16);
	}

	if (!(mem_mask & 0x0000ff00))	/* FM bank 2 address (OPL3/YMF 262 extended)*/
	{
		YMF278B_control_port_0_B_w(0, data>>8);
	}

	if (!(mem_mask & 0x000000ff))	/* FM bank 2 data*/
	{
		YMF278B_data_port_0_B_w(0, data);
	}
}

static WRITE32_HANDLER( psh_ymf_pcm_w )
{
	if (!(mem_mask & 0xff000000))	/* PCM address (OPL4/YMF 278B extended)*/
	{
		YMF278B_control_port_0_C_w(0, data>>24);

#if ROMTEST
		if (data>>24 == 0x06)	/* Reset Sample reading (They always write this code immediately before reading data)*/
		{
			sample_offs = 0;
		}
#endif
	}

	if (!(mem_mask & 0x00ff0000))	/* PCM data*/
	{
		YMF278B_data_port_0_C_w(0, data>>16);
	}
}

static MEMORY_READ32_START( ps4_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	/* program ROM (1 meg)*/
	{ 0x02000000, 0x021fffff, MRA32_BANK1 }, /* data ROM*/
	{ 0x03000000, 0x030037ff, MRA32_RAM },
	{ 0x03003fe0, 0x03003fe3, ps4_eeprom_r },
	{ 0x03003fe4, 0x03003fe7, MRA32_NOP }, /* also writes to this address - might be vblank?*/
/*	{ 0x03003fe8, 0x03003fef, MRA32_RAM }, */ /* vid regs?*/
	{ 0x03004000, 0x03005fff, MRA32_RAM },
	{ 0x05000000, 0x05000003, psh_ymf_fm_r }, /* read YMF status*/
	{ 0x05800000, 0x05800007, ps4_io32_r }, /* Screen 1+2's Controls*/
	{ 0x06000000, 0x060fffff, MRA32_RAM },	/* main RAM (1 meg)*/

#if ROMTEST
	{ 0x05000004, 0x05000007, ps4_sample_r }, /* data for rom tests (Used to verify Sample rom)*/
	{ 0x03006000, 0x03007fff, MRA32_BANK2 }, /* data for rom tests (gfx), data is controlled by vidreg*/
#endif
MEMORY_END

static MEMORY_WRITE32_START( ps4_writemem )
	{ 0x00000000, 0x000fffff, MWA32_ROM },	/* program ROM (1 meg)*/
	{ 0x03000000, 0x030037ff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x03003fe0, 0x03003fe3, ps4_eeprom_w },
/*	{ 0x03003fe4, 0x03003fe7, MWA32_NOP }, */ /* might be vblank?*/
	{ 0x03003fe4, 0x03003fef, ps4_vidregs_w, &psikyo4_vidregs }, /* vid regs?*/
	{ 0x03003ff0, 0x03003ff3, ps4_screen1_brt_w }, /* screen 1 brightness*/
	{ 0x03003ff4, 0x03003ff7, ps4_bgpen_1_dword_w, &bgpen_1 }, /* screen 1 clear colour*/
	{ 0x03003ff8, 0x03003ffb, ps4_screen2_brt_w }, /* screen 2 brightness*/
	{ 0x03003ffc, 0x03003fff, ps4_bgpen_2_dword_w, &bgpen_2 }, /* screen 2 clear colour*/
	{ 0x03004000, 0x03005fff, ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 }, /* palette*/
	{ 0x05000000, 0x05000003, psh_ymf_fm_w }, /* first 2 OPL4 register banks*/
	{ 0x05000004, 0x05000007, psh_ymf_pcm_w }, /* third OPL4 register bank*/
	{ 0x05800008, 0x0580000b, MWA32_RAM, &ps4_io_select }, /* Used by Mahjong games to choose input (also maps normal loderndf inputs to offsets)*/
	{ 0x06000000, 0x060fffff, MWA32_RAM, &ps4_ram },	/* work RAM*/
MEMORY_END

static void irqhandler(int linestate)
{
	if (linestate)
		cpu_set_irq_line(0, 12, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 12, CLEAR_LINE);
}

static struct YMF278B_interface ymf278b_interface =
{
	1,
	{ MASTER_CLOCK/2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) },
	{ irqhandler }
};

static MACHINE_DRIVER_START( ps4big )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, MASTER_CLOCK/2)
	MDRV_CPU_MEMORY(ps4_readmem,ps4_writemem)
	MDRV_CPU_VBLANK_INT(psikyosh_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C56)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_DUAL_MONITOR)
#if DUAL_SCREEN
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(80*8, 32*8)
	MDRV_VISIBLE_AREA(0, 80*8-1, 0, 28*8-1)
#else
	MDRV_ASPECT_RATIO(4,3)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 28*8-1)
#endif

	MDRV_GFXDECODE(gfxdecodeinfops4)
	MDRV_PALETTE_LENGTH((0x2000/4)*2 + 2) /* 0x2000/4 for each screen. 1 for each screen clear colour */

	MDRV_VIDEO_START(psikyo4)
	MDRV_VIDEO_UPDATE(psikyo4)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ps4small )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(ps4big)

#if DUAL_SCREEN
	MDRV_VISIBLE_AREA(0, 80*8-1, 0, 30*8-1)
#else
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 30*8-1)
#endif
MACHINE_DRIVER_END



#define UNUSED_PORT \
	PORT_START	/* not read? */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( hotgmck )
	PORT_START	/* IN0 fake player 1 controls 1st bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 fake player 1 controls 2nd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 fake player 1 controls 3rd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 C",     KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 G",     KEYCODE_G,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 K",     KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Chi",   KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 fake player 1 controls 4th bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 L",     KEYCODE_L,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 fake player 2 controls 1st bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 A",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 E",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 I",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 M",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Kan",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 fake player 2 controls 2nd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 B",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 F",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 J",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 N",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Reach", IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 fake player 2 controls 3rd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 C",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 G",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 K",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 Chi",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Ron",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 fake player 2 controls 4th bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 D",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 H",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 L",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 Pon",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN8 system inputs */
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) /* Screen 1*/
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_COIN2    ) /* Screen 2*/
	PORT_BIT(  0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) /* Screen 1*/
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Unknown effects */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#else
	PORT_BIT(  0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT(  0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) /* Screen 2*/

#if !DUAL_SCREEN
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL2 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif

    PORT_START/* jumper pads 'JP4' on the PCB */
	UNUSED_PORT
INPUT_PORTS_END

INPUT_PORTS_START( loderndf )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 ) /* Can be used as Retry button*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 ) /* Can be used as Retry button*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START /* IN3 system inputs */
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) /* Screen 1*/
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) /* Screen 1 - 2nd slot*/
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) /* Screen 2*/
	PORT_BIT(  0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) /* Screen 2 - 2nd slot*/
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) /* Screen 1*/
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Must be high for rom test, unknown other side-effects */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#else
	PORT_BIT(  0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT(  0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) /* Screen 2*/

	PORT_START	/* IN4 player 1 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 ) /* Can be used as Retry button*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN5 player 2 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 ) /* Can be used as Retry button*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START4 )

	UNUSED_PORT /* IN6 unused? */

	UNUSED_PORT /* IN7 unused? */

#if !DUAL_SCREEN
	UNUSED_PORT /* IN8 dummy, to pad below to IN9 */
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1+PL2 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL3+PL4 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif

	PORT_START/* jumper pads 'JP4' on the PCB */
/*  1-ON,2-ON,3-ON,4-ON  --> Japanese
    1-ON,2-ON,3-ON,4-OFF --> English */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan (Shows Version Number)" )
	PORT_DIPSETTING(    0x01, "World (Does Not Show Version Number)" )
INPUT_PORTS_END

/* unused inputs also act as duplicate buttons */
INPUT_PORTS_START( hotdebut )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START /* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) /* Screen 1*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) /* Screen 1 - 2nd slot*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) /* Screen 2*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) /* Screen 2 - 2nd slot*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) /* Screen 1*/
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Must be high for rom test, unknown other side-effects */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) /* Screen 2*/

	PORT_START	/* IN4 player 1 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN5 player 2 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START4 )

	UNUSED_PORT /* IN6 unused? */

	UNUSED_PORT /* IN7 unused? */

#if !DUAL_SCREEN
	UNUSED_PORT /* IN8 dummy, to pad below to IN9 */
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1+PL2 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL3+PL4 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif

    PORT_START/* jumper pads 'JP4' on the PCB */
	UNUSED_PORT
INPUT_PORTS_END

#if ROMTEST
#define ROMTEST_GFX 0
#else
#define ROMTEST_GFX ROMREGION_DISPOSE
#endif

ROM_START( hotgmck )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-u23.bin", 0x000002, 0x080000, CRC(23ed4aa5) SHA1(bb4f57a6adffc6336fc572a4ff1f5dfc284ee4fb) )
	ROM_LOAD32_WORD_SWAP( "1-u22.bin", 0x000000, 0x080000, CRC(5db3649f) SHA1(6ea7bd18bcf6224ed9b0480bb59c684f13b71d8a) )
	ROM_LOAD16_WORD_SWAP( "prog.bin",  0x100000, 0x200000, CRC(500f6b1b) SHA1(4ce454e44da08e351a81ca4b670ff3e080dcb330) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.bin", 0x0000000, 0x400000, CRC(91f9ba60) SHA1(968de0cd275784cf082df172b7f205861a8fbae4) )
	ROM_LOAD32_WORD( "0h.bin", 0x0000002, 0x400000, CRC(bfa800b7) SHA1(8e4a026f20b7ba035eb78713d64bfcb611c90640) )
	ROM_LOAD32_WORD( "1l.bin", 0x0800000, 0x400000, CRC(4b670809) SHA1(c0fe45d7618653b089ec293e7661da682b77534d) )
	ROM_LOAD32_WORD( "1h.bin", 0x0800002, 0x400000, CRC(ab513a4d) SHA1(4d589545765f55aaddb05f484fbf6248af3e237b) )
	ROM_LOAD32_WORD( "2l.bin", 0x1000000, 0x400000, CRC(1a7d51e9) SHA1(0722d89865cc2c9acda899bacb2787481c16c01a) )
	ROM_LOAD32_WORD( "2h.bin", 0x1000002, 0x400000, CRC(bf866222) SHA1(c70f4b0ff3997bf94a89cd613a2877f062014393) )
	ROM_LOAD32_WORD( "3l.bin", 0x1800000, 0x400000, CRC(a8a646f7) SHA1(be96626f3a4c8eb81f0bb7d8ac1c4e6619be50c8) )
	ROM_LOAD32_WORD( "3h.bin", 0x1800002, 0x400000, CRC(8c32becd) SHA1(9a8ddda4c6c007bb5cd4abb11859a4b7f1b1d578) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.bin", 0x000000, 0x400000, CRC(c090d51a) SHA1(d229753b536209fe0da1985ca694fd1a73bc0f39) )
	ROM_LOAD( "snd1.bin", 0x400000, 0x400000, CRC(c24243b5) SHA1(2100d5d7d2e4b9ed90bde38cb61a5da09f00ce21) )
ROM_END

ROM_START( hgkairak )
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u23",   0x000002, 0x080000, CRC(1c1a034d) SHA1(1be7793b1f9a0a738519b4b4f663b247011870db) )
	ROM_LOAD32_WORD_SWAP( "1.u22",   0x000000, 0x080000, CRC(24b04aa2) SHA1(b63d02fc15f03b93a74f5549fad236939905e382) )
	ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x100000, CRC(83cff542) SHA1(0ea5717e0b9e6c27aaf61f7e4909ed9a353b4d3b) )

	ROM_REGION( 0x3000000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(f7472212) SHA1(6f6c1a75615f6a1df4d9bc97225b8e1422eb114a) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(30019d0f) SHA1(1b6690ead9941171086afc89d95292c40348a15b) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(f46d5002) SHA1(0ce47b1c6da1a8ec3fd341d903d6a3e0447529e2) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(210592b6) SHA1(d98c3df589f5a707043979ede44c397705c13d11) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(b98adf21) SHA1(592b609f665a8a6af169611d6dbe7580df22e0c8) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(8e3da1e1) SHA1(9001d7484ad85d1febf9c1b925445246dfd66419) )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, CRC(fa7ba4ed) SHA1(cfe7651a48549a15f3fa81c744cf9204cd4f6f9a) )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, CRC(a5d400ea) SHA1(a550b30fa854bab11389a81fb73479bec0f4a5ff) )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, CRC(76c10026) SHA1(9cb2f29d123065d62a42743307db3f949432e2d5) )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, CRC(799f0889) SHA1(8495a15a2b2bd5d7324264b388f3b7e5a7d36cd6) )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, CRC(4639ef36) SHA1(324ffcfa1b1b9def00c15f628c59cea1d09b031d) )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, CRC(549e9e9e) SHA1(90c1695c89c059852f8b4f714b3dfee006839b44) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(0e8e5fdf) SHA1(041e3118f7a838dcc9fb99a1028fb48a452ba1d9) )
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(d8057d2f) SHA1(51d96cc4e9da81cbd1e815c652707407e6c7c3ae) )
ROM_END

ROM_START( hotgmck3 )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(3b06a4a3) SHA1(7363c2867367ca92a20fcb5ee1a5f1afbd785c63) )
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(7aad6b24) SHA1(160dfac94002766709369aad66d3b1b11d35ee63) )
	ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x100000, CRC(316c3356) SHA1(4664465c3f88d655379235881f1142a7954c80fc) )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(d4bbd035) SHA1(525739eafa4574541b217707514b256af588a996) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(e8832b0b) SHA1(aa13c264964b1c48094a303b18407a7873c60267) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(08426cb2) SHA1(a66463a93580fa7b730df7d0b72176daf6d0a6f2) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(112c6eea) SHA1(45d00427cec4f8ba98dc2f5b1e0d13d0f7ef56d1) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(0f197cd4) SHA1(ea1f20260470a5bee1076c3c243d7f57aabdaaba) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(fc37808c) SHA1(c35c666aad123ea1a37a5b4d6d51e55204c18e02) )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, CRC(c4d094dc) SHA1(bce585e41c9892be84a9fbdbb6d0f6b9baecaccf) )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, CRC(ef0dad0a) SHA1(f98032a8b7c3e17b52761ba98e46bdc5e0fa032a) )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, CRC(5186790f) SHA1(e2e2beb0dee856ec089c6a380442b4a1f40901d9) )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, CRC(187a6f43) SHA1(eedcc58ba59ffb0ccd7925edeb4532f8de357ec1) )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, CRC(ecf151f3) SHA1(56277c5d675e579e2148961aadc06019a64f3367) )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, CRC(720bf4ec) SHA1(214fb544ddc229485ae2a5ec34f95af9fa423a80) )
	ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x400000, CRC(e490404d) SHA1(dc3429ed248e7954c27eb6f29b392a35131592ba) )
	ROM_LOAD32_WORD( "6h.u17", 0x3000002, 0x400000, CRC(7e8a141a) SHA1(3f53e422a3ad2f2a02d17b2368b2d455c614bc04) )
	ROM_LOAD32_WORD( "7l.u9",  0x3800000, 0x400000, CRC(2ec78fb2) SHA1(194e9833ab7057c2f83c581e722b41631d99fccc) )
	ROM_LOAD32_WORD( "7h.u18", 0x3800002, 0x400000, CRC(c1735612) SHA1(84e32d3249d57cdc8ea91780801eaa196c439895) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(d62a0dba) SHA1(d81e2e1251b62eca8cd4d8eec2515b2cf7d7ff0a) )
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(1df91fb4) SHA1(f0f2d2d717fbd16a67da9f0e21f288ceedef839f) )
ROM_END

ROM_START( loderndf )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1b.u23", 0x000002, 0x080000, CRC(fae92286) SHA1(c3d3a50514fb9c0bbd3ffb5c4bfcc853dc1893d2) )
	ROM_LOAD32_WORD_SWAP( "2b.u22", 0x000000, 0x080000, CRC(fe2424c0) SHA1(48a329cfdf98da1a8701b430c159d470c0f5eca1) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, CRC(ccae855d) SHA1(1fc44e2a9d2ce2bca0a57e96140fbc80a0943141) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, CRC(7a146c59) SHA1(e4b30b5a826b8772d144d503e591d1d32783c016) )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, CRC(7a9cd21e) SHA1(dfb36625c2aae3e774ec2451051b7038e0767b6d) )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, CRC(78f40d0d) SHA1(243acb73a183a41a3e35a2c746ad31dd6fcd3ef4) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, CRC(2da3788f) SHA1(199d4d750a107cbdf8c16cd5b097171743769d9c) ) /* Fails hidden rom test*/
ROM_END

ROM_START( loderdfa )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "12.u23", 0x000002, 0x080000, CRC(661d372e) SHA1(c509c3ad9ca01e0f58bfc319b2738ecc36865ffd) )
	ROM_LOAD32_WORD_SWAP( "3.u22", 0x000000, 0x080000, CRC(0a63529f) SHA1(05dd7877041b69d46e41c5bddb877c083620294b) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, CRC(ccae855d) SHA1(1fc44e2a9d2ce2bca0a57e96140fbc80a0943141) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, CRC(7a146c59) SHA1(e4b30b5a826b8772d144d503e591d1d32783c016) )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, CRC(7a9cd21e) SHA1(dfb36625c2aae3e774ec2451051b7038e0767b6d) )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, CRC(78f40d0d) SHA1(243acb73a183a41a3e35a2c746ad31dd6fcd3ef4) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, CRC(2da3788f) SHA1(199d4d750a107cbdf8c16cd5b097171743769d9c) ) /* Fails hidden rom test*/
ROM_END

ROM_START( hotdebut )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(0b0d0027) SHA1(f62c487a725439af035d2904d453d3c2f7a5649b) )
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(c3b5180b) SHA1(615cc1fd99a1e4634b04bb92a3c41f914644e903) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(15da9983) SHA1(a96dd048080b5bb5ce903b5f72b3c24e89e1bee3) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(76d7b73f) SHA1(0682d4155ad61cab958d55b85914c69120d7d6fc) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(76ea3498) SHA1(ab2fb4008cf1e2b48a81306386cdc463b3bb4783) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(a056859f) SHA1(5821cbb263059a32c5b599da666fb59929d6326a) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(9d2d1bb1) SHA1(33b41aa50be3040871b6dc6faee0bd99c5e46cd3) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(a7753c4d) SHA1(adb33de478064cc9255d1bb5c63acc5d8bfbb8eb) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(eef28aa7) SHA1(d10d3f62a2e4c2a8e5fccece9c272f8ead50e5ed) )
ROM_END

/* are these right? should i fake the counter return?
   'speedups / idle skipping isn't needed for 'hotgmck, hgkairak'
   as the core catches and skips the idle loops automatically'
*/

static READ32_HANDLER( loderndf_speedup_r )
{
/*
PC  :00001B3C: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B3E: ADD     #$01,R3
PC  :00001B40: MOV.L   R3,@R14
PC  :00001B42: MOV.L   @($54,PC),R1
PC  :00001B44: MOV.L   @R1,R2
PC  :00001B46: TST     R2,R2
PC  :00001B48: BT      $00001B3C
*/

	if (activecpu_get_pc()==0x00001B3E) cpu_spinuntil_int();
	return ps4_ram[0x000020/4];
}

static READ32_HANDLER( loderdfa_speedup_r )
{
/*
PC  :00001B48: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B4A: ADD     #$01,R3
PC  :00001B4C: MOV.L   R3,@R14
PC  :00001B4E: MOV.L   @($54,PC),R1
PC  :00001B50: MOV.L   @R1,R2
PC  :00001B52: TST     R2,R2
PC  :00001B54: BT      $00001B48
*/

	if (activecpu_get_pc()==0x00001B4A) cpu_spinuntil_int();
	return ps4_ram[0x000020/4];
}

static READ32_HANDLER( hotdebut_speedup_r )
{
/*
PC  :000029EC: MOV.L   @R14,R2
PC  :000029EE: ADD     #$01,R2
PC  :000029F0: MOV.L   R2,@R14
PC  :000029F2: MOV.L   @($64,PC),R1
PC  :000029F4: MOV.L   @R1,R3
PC  :000029F6: TST     R3,R3
PC  :000029F8: BT      $000029EC
*/

	if (activecpu_get_pc()==0x000029EE) cpu_spinuntil_int();
	return ps4_ram[0x00001c/4];
}

static DRIVER_INIT( hotgmck )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler (0, 0x5800000, 0x5800007, hotgmck_io32_r ); /* Different Inputs*/
}

static DRIVER_INIT( loderndf )
{
	install_mem_read32_handler(0, 0x6000020, 0x6000023, loderndf_speedup_r );
}

static DRIVER_INIT( loderdfa )
{
	install_mem_read32_handler(0, 0x6000020, 0x6000023, loderdfa_speedup_r );
}

static DRIVER_INIT( hotdebut )
{
	install_mem_read32_handler(0, 0x600001c, 0x600001f, hotdebut_speedup_r );
}


/*     YEAR  NAME      PARENT    MACHINE    INPUT     INIT      MONITOR COMPANY   FULLNAME FLAGS */

GAMEX( 1997, hotgmck,  0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1998, hgkairak, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick Kairakuten (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1999, hotgmck3, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick 3 Digital Surfing (Japan)", GAME_IMPERFECT_SOUND )
GAME ( 2000, loderndf, 0,        ps4small,  loderndf, loderndf, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. B)" )
GAME ( 2000, loderdfa, loderndf, ps4small,  loderndf, loderdfa, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. A)" )
GAME ( 2000, hotdebut, 0,        ps4small,  hotdebut, hotdebut, ROT0,   "Psikyo / Moss", "Quiz de Idol! Hot Debut (Japan)" )
