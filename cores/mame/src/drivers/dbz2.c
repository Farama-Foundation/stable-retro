/*
  Dragonball Z
  Banpresto, 1993

  Dragonball Z 2 Super Battle
  (c) 1994 Banpresto

  Driver by David Haywood and R. Belmont

  MC68000 + Konami Xexex-era video hardware and system controller ICs
  Z80 + YM2151 + OKIM6295 for sound

  Status: DBZ2 is playable with music and sound and proper controls/dips.

  Note: game has an extremely complete test mode, it's beautiful for emulation.
        flip the DIP and check it out!

  TODO:
	- Gfx priorities (note that it has two 053251)
	- Self Test Fails
	- Some offsets/colours in DBZ1

PCB Layout:

BP924-1  PWB250248D (note PCB is identical to DBZ2 also)
|-------------------------------------------------------|
| YM3014  Z80    32MHz  053252       222A05   222A07    |
|   YM2151 5168                      222A04   222A06    |
| 1.056kHz 5168                                         |
|  M6295   222A10                           5864        |
|   222A03     68000                 2018   5864 053246A|
|J          222A11  222A12           2018               |
|A 5864        *       *                                |
|M 5864     62256   62256                               |
|M                                               053247A|
|A                                                      |
|                                053936 053936     2018 |
|    053251  053251                                2018 |
|                                        CY7C128        |
|       054157  054156  5864     CY7C128 CY7C128 CY7C128|
|       222A01  222A02  5864     CY7C128                |
| DSW2  DSW1            5864     CY7C128 222A08  222A09 |
|                                           *       *   |
|-------------------------------------------------------|

Notes:
      68k clock: 16.000MHz
      Z80 clock: 4.000MHz
   YM2151 clock: 4.000MHz
    M6295 clock: 1.056MHz (sample rate = /132)
          Vsync: 55Hz
          Hsync: 15.36kHz
              *: unpopulated ROM positions on DBZ

*/

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"

/* BG LAYER */

data16_t* dbz2_bg_videoram;
data16_t* dbz2_bg2_videoram;

WRITE16_HANDLER(dbz2_bg_videoram_w);
WRITE16_HANDLER(dbz2_bg2_videoram_w);

static int dbz2_control;

VIDEO_START(dbz);
VIDEO_START(dbz2);
VIDEO_UPDATE(dbz);
VIDEO_UPDATE(dbz2);

static INTERRUPT_GEN(dbz2_interrupt)
{
	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);
			break;

		case 1:
			if (K053246_is_IRQ_enabled())
				cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
			break;
	}
}

#if 0
static READ16_HANDLER(dbzcontrol_r)
{
	return dbz2_control;
}
#endif

static WRITE16_HANDLER(dbzcontrol_w)
{
	/* bit 10 = enable '246 readback */

	COMBINE_DATA(&dbz2_control);

	if (data & 0x400)
	{
		K053246_set_OBJCHA_line(ASSERT_LINE);
	}
	else
	{
		K053246_set_OBJCHA_line(CLEAR_LINE);
	}
}

static READ16_HANDLER(dbz2_inp0_r)
{
	return readinputport(0) | (readinputport(1)<<8);
}

static READ16_HANDLER(dbz2_inp1_r)
{
	return readinputport(3) | (readinputport(2)<<8);
}

static READ16_HANDLER(dbz2_inp2_r)
{
	return readinputport(4) | (readinputport(4)<<8);
}

static WRITE16_HANDLER( dbz2_sound_command_w )
{
	soundlatch_w(0, data>>8);
}

static WRITE16_HANDLER( dbz2_sound_cause_nmi )
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

static void dbz2_sound_irq(int irq)
{
	if (irq)
		cpu_set_irq_line(1, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(1, 0, CLEAR_LINE);
}

static MEMORY_READ16_START( dbz2readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x480000, 0x48ffff, MRA16_RAM },
	{ 0x490000, 0x491fff, K054157_ram_word_r },	/* '157 RAM is mirrored twice*/
	{ 0x492000, 0x493fff, K054157_ram_word_r },
	{ 0x498000, 0x49ffff, K054157_rom_word_8000_r },	/* code near a60 in dbz2, subroutine at 730 in dbz*/
	{ 0x4a0000, 0x4a0fff, K053247_word_r },
	{ 0x4a1000, 0x4a3fff, MRA16_RAM },
	{ 0x4a8000, 0x4abfff, MRA16_RAM },			/* palette*/
	{ 0x4c0000, 0x4c0001, K053246_word_r },
	{ 0x4e0000, 0x4e0001, dbz2_inp0_r },
	{ 0x4e0002, 0x4e0003, dbz2_inp1_r },
	{ 0x4e4000, 0x4e4001, dbz2_inp2_r },
	{ 0x500000, 0x501fff, MRA16_RAM },
	{ 0x508000, 0x509fff, MRA16_RAM },
	{ 0x510000, 0x513fff, MRA16_RAM },
	{ 0x518000, 0x51bfff, MRA16_RAM },
	{ 0x600000, 0x6fffff, MRA16_NOP }, 			/* PSAC 1 ROM readback window*/
	{ 0x700000, 0x7fffff, MRA16_NOP }, 			/* PSAC 2 ROM readback window*/
MEMORY_END

static MEMORY_WRITE16_START( dbz2writemem )
	{ 0x480000, 0x48ffff, MWA16_RAM },
	{ 0x490000, 0x491fff, K054157_ram_word_w },
	{ 0x492000, 0x493fff, K054157_ram_word_w },
	{ 0x4a0000, 0x4a0fff, K053247_word_w },
	{ 0x4a1000, 0x4a3fff, MWA16_RAM },
	{ 0x4a8000, 0x4abfff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0x4c0000, 0x4c0007, K053246_word_w },
	{ 0x4c4000, 0x4c4007, K053246_word_w },
	{ 0x4c8000, 0x4c8007, K054157_b_word_w },
	{ 0x4cc000, 0x4cc03f, K054157_word_w },
	{ 0x4ec000, 0x4ec001, dbzcontrol_w },
	{ 0x4d0000, 0x4d001f, MWA16_RAM, &K053936_0_ctrl },
	{ 0x4d4000, 0x4d401f, MWA16_RAM, &K053936_1_ctrl },
	{ 0x4e8000, 0x4e8001, MWA16_NOP },
	{ 0x4f0000, 0x4f0001, dbz2_sound_command_w },
	{ 0x4f4000, 0x4f4001, dbz2_sound_cause_nmi },
	{ 0x4f8000, 0x4f801f, MWA16_NOP },			/* 251 #1*/
	{ 0x4fc000, 0x4fc01f, K053251_lsb_w },		/* 251 #2*/
	{ 0x500000, 0x501fff, dbz2_bg2_videoram_w, &dbz2_bg2_videoram },
	{ 0x508000, 0x509fff, dbz2_bg_videoram_w, &dbz2_bg_videoram },
	{ 0x510000, 0x513fff, MWA16_RAM, &K053936_0_linectrl }, /* ?? guess, it might not be*/
	{ 0x518000, 0x51bfff, MWA16_RAM, &K053936_1_linectrl }, /* ?? guess, it might not be*/
MEMORY_END

/* dbz2 sound */
/* IRQ: from YM2151.  NMI: from 68000.  Port 0: write to ack NMI */

static MEMORY_READ_START( dbz2sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc001, YM2151_status_port_0_r },
	{ 0xd000, 0xd002, OKIM6295_status_0_r },
	{ 0xe000, 0xe001, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( dbz2sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd001, OKIM6295_data_0_w },
MEMORY_END

static PORT_READ_START( dbz2sound_readport )
PORT_END

static PORT_WRITE_START( dbz2sound_writeport )
	{ 0x00, 0x00, IOWP_NOP },
PORT_END

/**********************************************************************************/


INPUT_PORTS_START( dbz )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) /* I think this is right, but can't stomach the game long enough to check*/
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* seems unused*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) ) /* Definitely correct */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Service_Mode) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, "Mask ROM Test" ) /*NOP'd*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Test Switch", KEYCODE_F2, IP_JOY_NONE )				/* "Test"*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Disabled" )*/
INPUT_PORTS_END

INPUT_PORTS_START( dbz2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) 
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Level Select" ) 
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Service_Mode ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, "Mask ROM Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Test Switch", KEYCODE_F2, IP_JOY_NONE )		/* "Test"*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Disabled" )*/
INPUT_PORTS_END

/**********************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,	/* total guess */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ dbz2_sound_irq }
};

static struct OKIM6295interface m6295_interface =
{
	1,  /* 1 chip */
	{ 1056000/132 },	/* confirmed */
	{ REGION_SOUND1 },
	{ 100 }
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
	{ REGION_GFX3, 0, &bglayout,     0x400, 64 },
	{ REGION_GFX4, 0, &bglayout,	 0x800, 64 },
	{ -1 } /* end of array */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( dbz2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)
	MDRV_CPU_MEMORY(dbz2readmem,dbz2writemem)
	MDRV_CPU_VBLANK_INT(dbz2_interrupt,2)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dbz2sound_readmem, dbz2sound_writemem)
	MDRV_CPU_PORTS(dbz2sound_readport,dbz2sound_writeport)

	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 48*8-1, 0, 32*8-1 )
	MDRV_VIDEO_START(dbz2)
	MDRV_VIDEO_UPDATE(dbz2)
	MDRV_PALETTE_LENGTH(0x4000/2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, m6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dbz )
	MDRV_IMPORT_FROM(dbz2)

	MDRV_VIDEO_START(dbz)
	MDRV_VIDEO_UPDATE(dbz)
	MDRV_VISIBLE_AREA(34, 34+48*8-1, 0, 32*8-1 )
MACHINE_DRIVER_END

/**********************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START( dbz )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "222a11.9e", 0x000000, 0x80000, CRC(60c7d9b2) SHA1(718ef89e89b3943845e91bedfc5c1d26229f9fe5) )
	ROM_LOAD16_BYTE( "222a12.9f", 0x000001, 0x80000, CRC(6ebc6853) SHA1(e9b2068246228968cc6b8554215563cacaa5ba9f) )

	ROM_REGION( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "222a11.9e", 0x000000, 0x80000, CRC(60c7d9b2) SHA1(718ef89e89b3943845e91bedfc5c1d26229f9fe5) )
	ROM_LOAD16_BYTE( "222a12.9f", 0x000001, 0x80000, CRC(6ebc6853) SHA1(e9b2068246228968cc6b8554215563cacaa5ba9f) )

	/* sound program */
	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD("222a10.5e", 0x000000, 0x08000, CRC(1c93e30a) SHA1(8545a0ac5126b3c855e1901b186f57820699895d) )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "222a01.27c", 0x000000, 0x200000, CRC(9fce4ed4) SHA1(81e19375b351ee247f066434dd595149333d73c5) )
	ROM_LOAD( "222a02.27e", 0x200000, 0x200000, CRC(651acaa5) SHA1(33942a90fb294b5da6a48e5bfb741b31babca188) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "222a04.3j", 0x000000, 0x200000, CRC(2533b95a) SHA1(35910836b6030130d742eae6c4bf1cdf1ff43fa4) )
	ROM_LOAD64_WORD( "222a05.1j", 0x000002, 0x200000, CRC(731b7f93) SHA1(b676fff2ede5aa72c49fe12736cd60766462fe0b) )
	ROM_LOAD64_WORD( "222a06.3l", 0x000004, 0x200000, CRC(97b767d3) SHA1(3d879c431586da2f88c632ab1a531b4a5ec96939) )
	ROM_LOAD64_WORD( "222a07.1l", 0x000006, 0x200000, CRC(430bc873) SHA1(ea483195bb7f20ef3df7cfba153e5f6f8d53e5f9) )

	/* K053536 PSAC-2 #1 */
	ROM_REGION( 0x200000, REGION_GFX3, 0)
	ROM_LOAD( "222a08.25k", 0x000000, 0x200000, CRC(6410ee1b) SHA1(2296aafd3ba25f63a12130f7b58de53e88f14e92) )

	/* K053536 PSAC-2 #2 */
	ROM_REGION( 0x200000, REGION_GFX4, 0)
	ROM_LOAD( "222a09.25l", 0x000000, 0x200000, CRC(f7b3f070) SHA1(50ebd8cfcda292a3df5664de50f9212108d58923) )

	/* sound data */
	ROM_REGION( 0x40000, REGION_SOUND1, 0)
	ROM_LOAD( "222a03.7c", 0x000000, 0x40000, CRC(1924467b) SHA1(57922090509bcc63b4783e8f2c5e95afd2090b87) )
ROM_END

ROM_START( dbz2 )
	/* main program */
	ROM_REGION( 0x400000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "a9e.9e", 0x000000, 0x80000, CRC(e6a142c9) SHA1(7951c8f7036a67a0cd3260f434654820bf3e603f) )
	ROM_LOAD16_BYTE( "a9f.9f", 0x000001, 0x80000, CRC(76cac399) SHA1(af6daa1f8b87c861dc62adef5ca029190c3cb9ae) )

	/* sound program */
	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD("s-001.5e", 0x000000, 0x08000, CRC(154e6d03) SHA1(db15c20982692271f40a733dfc3f2486221cd604) )

	/* tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "ds-b01.27c", 0x000000, 0x200000, CRC(8dc39972) SHA1(c6e3d4e0ff069e08bdb68e2b0ad24cc7314e4e93) )
	ROM_LOAD( "ds-b02.27e", 0x200000, 0x200000, CRC(7552f8cd) SHA1(1f3beffe9733b1a18d44b5e8880ff1cc97e7a8ab) )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "ds-o01.3j", 0x000000, 0x200000, CRC(d018531f) SHA1(d4082fe28e9f1f3f35aa75b4be650cadf1cef192) )
	ROM_LOAD64_WORD( "ds-o02.1j", 0x000002, 0x200000, CRC(5a0f1ebe) SHA1(3bb9e1389299dc046a24740ef1a1c543e44b5c37) )
	ROM_LOAD64_WORD( "ds-o03.3l", 0x000004, 0x200000, CRC(ddc3bef1) SHA1(69638ef53f627a238a12b6c206d57faadf894893) )
	ROM_LOAD64_WORD( "ds-o04.1l", 0x000006, 0x200000, CRC(b5df6676) SHA1(194cfce460ccd29e2cceec577aae4ec936ae88e5) )

	/* K053536 PSAC-2 #1 */
	ROM_REGION( 0x400000, REGION_GFX3, 0)
	ROM_LOAD( "ds-p01.25k", 0x000000, 0x200000, CRC(1c7aad68) SHA1(a5296cf12cec262eede55397ea929965576fea81) )
	ROM_LOAD( "ds-p02.27k", 0x200000, 0x200000, CRC(e4c3a43b) SHA1(f327f75fe82f8aafd2cfe6bdd3a426418615974b) )

	/* K053536 PSAC-2 #2 */
	ROM_REGION( 0x400000, REGION_GFX4, 0)
	ROM_LOAD( "ds-p03.25l", 0x000000, 0x200000, CRC(1eaa671b) SHA1(1875eefc6f2c3fc8feada56bfa6701144e8ef64b) )
	ROM_LOAD( "ds-p04.27l", 0x200000, 0x200000, CRC(5845ff98) SHA1(73b4c3f439321ce9c462119fe933e7cbda8cd498) )

	/* sound data */
	ROM_REGION( 0x40000, REGION_SOUND1, 0)
	ROM_LOAD( "pcm.7c", 0x000000, 0x40000, CRC(b58c884a) SHA1(0e2a7267e9dff29c9af25558081ec9d56629bc43) )
ROM_END


static DRIVER_INIT(dbz2)
{
	konami_rom_deinterleave_2(REGION_GFX1);
}

static DRIVER_INIT(dbz)
{
	data16_t *ROM;

	konami_rom_deinterleave_2(REGION_GFX1);

	ROM = (data16_t *)memory_region(REGION_CPU1);

	/* nop out dbz1's mask rom test*/
	/* tile ROM test*/
	ROM[0x790/2] = 0x4e71;
	ROM[0x792/2] = 0x4e71;
	/* PSAC2 ROM test*/
	ROM[0x982/2] = 0x4e71;
	ROM[0x984/2] = 0x4e71;
	ROM[0x986/2] = 0x4e71;
	ROM[0x988/2] = 0x4e71;
	ROM[0x98a/2] = 0x4e71;
	ROM[0x98c/2] = 0x4e71;
	ROM[0x98e/2] = 0x4e71;
	ROM[0x990/2] = 0x4e71;
}

GAMEX( 1993, dbz,  0, dbz,  dbz, dbz, ROT0, "Banpresto", "Dragonball Z" , GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, dbz2, 0, dbz2, dbz2, dbz2, ROT0, "Banpresto", "Dragonball Z 2 Super Battle" , GAME_IMPERFECT_GRAPHICS )
