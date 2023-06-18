#define RNG_DEBUG 0

/*
   Run and Gun / Slam Dunk
   (c) 1993 Konami

   Driver by R. Belmont.

   This hardware uses the 55673 sprite chip like PreGX and System GX, but in a 4 bit
   per pixel layout.  There is also an all-TTL front overlay tilemap and a rotating
   scaling background done with the PSAC2 ('936).

   Status: Front tilemap should be complete, sprites are mostly correct, controls
   should be fine.


   Change Log:

   (AT070703)
   drivers\rungun.c (this file)
     - mem maps, device settings, component communications, I/O's, sound...etc.

   vidhrdw\rungun.c
     - general clean-up, clipping, alignment

   vidhrdw\konamiic.c
     - missing sprites and priority

   Known Issues:
     - ** sound program ROM "247-a05" needs redump **
     - no dual monitor support
     - synchronization and other oddities (rungunu doesn't show attract mode)
     - swapped P12 and P34 controls in 4-player mode team selectet (real puzzler)
     - P3 and P4 coin chutes not working in 4-player mode
     - sprite palettes are not entirely right
     - ROZ update causes music to stutter
*/

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"

#include "mame2003.h"
#include "bootstrap.h"
#include "inptport.h"

VIDEO_START( rng );
VIDEO_UPDATE( rng );
MACHINE_INIT( rng );
READ16_HANDLER( ttl_ram_r );
WRITE16_HANDLER( ttl_ram_w );
WRITE16_HANDLER( rng_936_videoram_w );

data16_t *rng_936_videoram;

static data16_t *rng_sysreg;
static int init_eeprom_count;
static int rng_z80_control;
static int rng_sound_status;

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

static NVRAM_HANDLER( rungun )
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
		{
			init_eeprom_count = 10;
		}
	}
}

static READ16_HANDLER( rng_sysregs_r )
{
	data16_t data = 0;

	switch (offset)
	{
		case 0x00/2:
			if (readinputport(1) & 0x20)
				return(readinputport(2) | readinputport(4)<<8);
			else
			{
				data = readinputport(2) & readinputport(4);
				return(data<<8 | data);
			}
		break;

		case 0x02/2:
			if (readinputport(1) & 0x20)
				return(readinputport(3) | readinputport(5)<<8);
			else
			{
				data = readinputport(3) & readinputport(5);
				return(data<<8 | data);
			}
		break;

		case 0x04/2:
			/*
				bit0-7: coin mechs and services
				bit8 : freeze
				bit9 : joysticks layout(auto detect???)
			*/
			return(input_port_0_word_r(0, 0));
		break;

		case 0x06/2:
			if (ACCESSING_LSB)
			{
				data = readinputport(1) | EEPROM_read_bit();

				if (init_eeprom_count)
				{
					init_eeprom_count--;
					data &= 0xf7;
				}
			}
			return((rng_sysreg[0x06/2] & 0xff00) | data);
		break;
	}

	return(rng_sysreg[offset]);
}

static WRITE16_HANDLER( rng_sysregs_w )
{
	COMBINE_DATA(rng_sysreg + offset);

	switch (offset)
	{
		case 0x08/2:
			/*
				bit0  : EEPROM_write_bit
				bit1  : EEPROM_set_cs_line
				bit2  : EEPROM_set_clock_line
				bit3  : coin counter?
				bit7  : set before massive memory writes
				bit10 : IRQ5 ACK
			*/
			if (ACCESSING_LSB)
			{
				EEPROM_write_bit((data & 0x01) ? 1 : 0);
				EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
				EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);
			}

			if (!(data & 0x40))
				cpu_set_irq_line(0, MC68000_IRQ_5, CLEAR_LINE);
		break;

		case 0x0c/2:
			/*
				bit 0 : also enables IRQ???
				bit 1 : disable PSAC2 input?
				bit 2 : OBJCHA
				bit 3 : enable IRQ 5
			*/
			K053246_set_OBJCHA_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);
		break;
	}
}

static WRITE16_HANDLER( sound_cmd1_w )
{
	if (ACCESSING_MSB)
		soundlatch_w(0, data>>8);
}

static WRITE16_HANDLER( sound_cmd2_w )
{
	if (ACCESSING_MSB)
		soundlatch2_w(0, data>>8);
}

static WRITE16_HANDLER( sound_irq_w )
{
	if (ACCESSING_MSB)
		cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( sound_status_msb_r )
{
	if (ACCESSING_MSB)
		return(rng_sound_status<<8);

	return(0);
}

static INTERRUPT_GEN(rng_interrupt)
{
	if (rng_sysreg[0x0c/2] & 0x09)
		cpu_set_irq_line(0, MC68000_IRQ_5, ASSERT_LINE);
}

static MEMORY_READ16_START( rngreadmem )
	{ 0x000000, 0x2fffff, MRA16_ROM },		/* main program + data */
	{ 0x300000, 0x3007ff, MRA16_RAM },		/* palette RAM */
	{ 0x380000, 0x39ffff, MRA16_RAM },		/* work RAM */
	{ 0x400000, 0x43ffff, MRA16_NOP },		/* K053936_0_rom_r }, */ /* '936 ROM readback window */
	{ 0x480000, 0x48001f, rng_sysregs_r },
	{ 0x4c0000, 0x4c001f, K053252_word_r },	/* CCU (for scanline and vblank polling) */
	{ 0x580014, 0x580015, sound_status_msb_r },
	{ 0x580000, 0x58001f, MRA16_RAM },		/* sound regs read fall-through */
	{ 0x5c0000, 0x5c000d, K053246_word_r },	/* 246A ROM readback window */
	{ 0x600000, 0x600fff, K053247_word_r },	/* OBJ RAM */
	{ 0x601000, 0x601fff, MRA16_RAM },		/* communication? second monitor buffer? */
	{ 0x6c0000, 0x6cffff, MRA16_RAM },		/* PSAC2 render RAM */
	{ 0x700000, 0x7007ff, MRA16_RAM },		/* PSAC2 line effect */
	{ 0x740000, 0x741fff, ttl_ram_r },		/* text plane RAM */
#if RNG_DEBUG
	{ 0x5c0010, 0x5c001f, K053247_reg_word_r },
	{ 0x640000, 0x640007, K053246_reg_word_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( rngwritemem )
	{ 0x000000, 0x2fffff, MWA16_ROM },
	{ 0x300000, 0x3007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x380000, 0x39ffff, MWA16_RAM },		/* work RAM */
	{ 0x480000, 0x48001f, rng_sysregs_w, &rng_sysreg },
	{ 0x4c0000, 0x4c001f, K053252_word_w },	/* CCU */
	{ 0x540000, 0x540001, sound_irq_w },
	{ 0x58000c, 0x58000d, sound_cmd1_w },
	{ 0x58000e, 0x58000f, sound_cmd2_w },
	{ 0x580000, 0x58001f, MWA16_RAM },		/* sound regs write fall-through */
	{ 0x5c0010, 0x5c001f, K053247_reg_word_w },
	{ 0x600000, 0x600fff, K053247_word_w },	/* OBJ RAM */
	{ 0x601000, 0x601fff, MWA16_RAM },		/* communication? second monitor buffer? */
	{ 0x640000, 0x640007, K053246_word_w },	/* '246A registers */
	{ 0x680000, 0x68001f, MWA16_RAM, &K053936_0_ctrl },				/* '936 registers */
	{ 0x6c0000, 0x6cffff, rng_936_videoram_w, &rng_936_videoram },	/* PSAC2 ('936) RAM (34v + 35v) */
	{ 0x700000, 0x7007ff, MWA16_RAM, &K053936_0_linectrl },			/* "Line RAM" */
	{ 0x740000, 0x741fff, ttl_ram_w },		/* text plane RAM */
	{ 0x7c0000, 0x7c0001, MWA16_NOP },		/* watchdog */
MEMORY_END

/**********************************************************************************/

static WRITE_HANDLER( sound_status_w )
{
	rng_sound_status = data;
}

static WRITE_HANDLER( z80ctrl_w )
{
	rng_z80_control = data;

	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + (data & 0x07) * 0x4000);

	if (data & 0x10)
		cpu_set_nmi_line(1, CLEAR_LINE);
}

static INTERRUPT_GEN(audio_interrupt)
{
	if (rng_z80_control & 0x80) return;

	cpu_set_nmi_line(1, ASSERT_LINE);
}

/* sound (this should be split into sndhrdw/xexex.c or pregx.c or so someday) */

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
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, sound_status_w },
	{ 0xf800, 0xf800, z80ctrl_w },
	{ 0xfff0, 0xfff3, MWA_NOP },
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

static struct GfxLayout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bglayout, 0x0000, 64 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( rng )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)
	MDRV_CPU_MEMORY(rngreadmem,rngwritemem)
	MDRV_CPU_VBLANK_INT(rng_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 10000000) /* 8Mhz (10Mhz is much safer in self-test due to heavy sync) */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(audio_interrupt, 480)

	MDRV_INTERLEAVE(100) /* higher if sound stutters */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_MACHINE_INIT(rng)
	MDRV_NVRAM_HANDLER(rungun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(88, 88+384-1, 24, 24+224-1)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(rng)
	MDRV_VIDEO_UPDATE(rng)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END

INPUT_PORTS_START( rng )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )
	PORT_DIPNAME( 0x0100, 0x0000, "Freeze" )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0100, DEF_STR( On ) )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Monitors" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPNAME( 0x20, 0x20, "Number of players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Sound Output" )
	PORT_DIPSETTING(    0x40, "Mono" )
	PORT_DIPSETTING(    0x00, "Stereo" )
	PORT_DIPNAME( 0x04, 0x04, "Bit2 (Unknown)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit7 (Unknown)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

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

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START( rungunu )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247b03", 0x000000, 0x80000, CRC(f259fd11) SHA1(60381a3fa7f78022dcb3e2f3d13ea32a10e4e36e) )
	ROM_LOAD16_BYTE( "247b04", 0x000001, 0x80000, CRC(b918cf5a) SHA1(4314c611ef600ec081f409c78218de1639f8b463) )

	/* data */
	ROM_LOAD16_BYTE( "247a01", 0x100000, 0x80000, CRC(8341cf7d) SHA1(372c147c4a5d54aed2a16b0ed258247e65dda563) )
	ROM_LOAD16_BYTE( "247a02", 0x100001, 0x80000, CRC(f5ef3f45) SHA1(2e1d8f672c130dbfac4365dc1301b47beee10161) )

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	ROM_LOAD("247a05", 0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(        0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247a13", 0x000000, 0x200000, CRC(c5a8ef29) SHA1(23938b8093bc0b9eef91f6d38127ca7acbdc06a6) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )	/* 5y */
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )	/* 2u */
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )	/* 2y */
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )	/* 5u */

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )
ROM_END

ROM_START( rungun )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247-c03", 0x000000, 0x80000, CRC(fec3e1d6) SHA1(cd89dc32ad06308134d277f343a7e8b5fe381f69) )
	ROM_LOAD16_BYTE( "247-c04", 0x000001, 0x80000, CRC(1b556af9) SHA1(c8351ebd595307d561d089c66cd6ed7f6111d996) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	/* bad dump (higher banks and the second half of lower banks filled with 0xff) */
	ROM_LOAD("247-a05", 0x000000, 0x20000, BAD_DUMP CRC(412fa1e0) SHA1(3fcf203cfcfb7ec9539d8613a8bf95747c76cc4f) )
	/* borrowed from rungunu */
	ROM_LOAD("247a05",  0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(         0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, CRC(cc194089) SHA1(b5af94f5f583d282ac1499b371bbaac8b2fedc03) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )	/* 5y */
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )	/* 2u */
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )	/* 2y */
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )	/* 5u */

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )
ROM_END

ROM_START( slmdunkj )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247jaa03.bin", 0x000000, 0x20000, CRC(87572078) SHA1(cfa784eb40ed8b3bda9d57abb6022bbe92056206) )
	ROM_LOAD16_BYTE( "247jaa04.bin", 0x000001, 0x20000, CRC(aa105e00) SHA1(617ac14535048b6e0da43cc98c4b67c8e306bef1) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, REGION_CPU2, 0 )
	/* bad dump (higher banks and the second half of lower banks filled with 0xff) */
	ROM_LOAD("247-a05", 0x000000, 0x20000, BAD_DUMP CRC(412fa1e0) SHA1(3fcf203cfcfb7ec9539d8613a8bf95747c76cc4f) )
	/* borrowed from rungunu */
	ROM_LOAD("247a05",  0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(         0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, CRC(cc194089) SHA1(b5af94f5f583d282ac1499b371bbaac8b2fedc03) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )	/* 5y */
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )	/* 2u */
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )	/* 2y */
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )	/* 5u */

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )
ROM_END

static DRIVER_INIT( rng )
{
	K054539_init_flags(K054539_REVERSE_STEREO);
}

MACHINE_INIT( rng )
{
	memset(rng_sysreg, 0, 0x20);

	init_eeprom_count = 0;
	rng_z80_control = 0;
	rng_sound_status = 0;
}

GAMECX( 1993, rungun,   0,      rng, rng, rng, ROT0, "Konami", "Run and Gun (World ver. EAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND, &generic_ctrl, &rungun_bootstrap )
GAMECX( 1993, rungunu,  rungun, rng, rng, rng, ROT0, "Konami", "Run and Gun (US ver. UAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND, &generic_ctrl, &rungunu_bootstrap )
GAMEX( 1993, slmdunkj, rungun, rng, rng, rng, ROT0, "Konami", "Slam Dunk (Japan ver. JAA))", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
