/***************************************************************************

Taito Dual Screen Games
=======================

Darius 2      (c) 1989 Taito
Warrior Blade (c) 1991 Taito

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

The dual screen games operate on hardware with various similarities to
the Taito F2 system, as they share some custom ics e.g. the TC0100SCN.

For each screen the games have 3 separate layers of graphics: - one
128x64 tiled scrolling background plane of 8x8 tiles, a similar
foreground plane, and a 128x32 text plane with character definitions
held in ram. As well as this, there is a single sprite plane which
covers both screens. The sprites are 16x16 and are not zoomable.

Writing to the first TC0100SCN "writes through" to the subsidiary one
so both have identical contents. The only time the second TC0100SCN is
addressed on its own is during initial memory checks, I think. (?)

Warrior Blade has a slightly different gfx set for the 2nd screen
because the programmers ran out of scr gfx space (only 0xffff tiles
can be addressed by the TC0100SCN). In-game while tiles are
scrolling from one screen to the other it is necessary to have
identical gfx tiles for both screens. But for static screens (e.g. cut
scenes between levels) the gfx tiles needn't be the same. By
exploiting this they squeezed some extra graphics into the game.

There is a single 68000 processor which takes care of everything
except sound. That is done by a Z80 controlling a YM2610. Sound
commands are written to the Z80 by the 68000.


Tilemaps
========

TC0100SCN has tilemaps twice as wide as usual. The two BG tilemaps take
up twice the usual space, $8000 bytes each. The text tilemap takes up
the usual space, as its height is halved.

The double palette generator (one for each screen) is probably just a
result of the way the hardware works: they both have the same colors.


Dumper's Info
-------------

Darius II (Dual Screen Old & New JPN Ver.)
(c)1989 Taito
J1100204A
K1100483A

CPU 	:MC68000P12F(16MHz),Z80A
Sound	:YM2610
OSC 	:26.686MHz,16.000MHz
Other	:
TC0140SYT
TC0220IOC
TC0110PCR x2
TC0100SCN x2
TC0390LHC-1
TC0130LNB x8

Warrior Blade (JPN Ver.)
(c)1991 Taito
J1100295A
K1100710A (Label K11J0710A)

CPU 	:MC68000P12F(16MHz),Z80A
Sound	:YM2610B
OSC 	:26.686MHz,16.000MHz
Other	:
TC0140SYT
TC0510NIO
TC0110PCR x2
TC0100SCN x2
TC0390LHC-1
TC0130LNB x8

[The 390LHC/130LNB functions are unknown].

TODO
====

Unknown sprite bits.


Darius 2
--------

The unpleasant sounds when some big enemies appear are wrong: they
are meant to create rumbling on a subwoofer in the cabinet, a sort of
vibration device. They still affect the other channels despite
filtering above 20Hz.


Warriorb
--------

Colscroll effects?

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

MACHINE_INIT( taito_dualscreen );

VIDEO_START( darius2d );
VIDEO_START( warriorb );
VIDEO_UPDATE( warriorb );


/***********************************************************
                          SOUND
***********************************************************/

static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 7;
	reset_sound_region();
}

static WRITE16_HANDLER( warriorb_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);
}

static READ16_HANDLER( warriorb_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


static int ninjaw_pandata[4];		/**** sound pan control ****/

WRITE_HANDLER( warriorb_pancontrol )
{
	offset = offset&3;
	ninjaw_pandata[offset] = (data<<1) + data;   /* original volume*3 */

/*	usrintf_showmessage(" pan %02x %02x %02x %02x", ninjaw_pandata[0], ninjaw_pandata[1], ninjaw_pandata[2], ninjaw_pandata[3] );*/

	if (offset < 2)
		mixer_set_stereo_volume( 3, ninjaw_pandata[0], ninjaw_pandata[1] );
	else
		mixer_set_stereo_volume( 4, ninjaw_pandata[2], ninjaw_pandata[3] );
}



/***********************************************************
                      MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( darius2d_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x213fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_r },
	{ 0x240000, 0x253fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x260000, 0x26000f, TC0100SCN_ctrl_word_1_r },
	{ 0x400000, 0x400007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MRA16_RAM },	/* sprite ram */
	{ 0x800000, 0x80000f, TC0220IOC_halfword_r },
	{ 0x830000, 0x830003, warriorb_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( darius2d_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x213fff, TC0100SCN_dual_screen_w },	/* tilemaps (all screens) */
	{ 0x214000, 0x2141ff, MWA16_NOP },	/* error in screen clearing code ? */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_w },
	{ 0x240000, 0x253fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x260000, 0x26000f, TC0100SCN_ctrl_word_1_w },
	{ 0x400000, 0x400007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x80000f, TC0220IOC_halfword_w },
/*	{ 0x820000, 0x820001, MWA16_NOP },	*/ /* ???*/
	{ 0x830000, 0x830003, warriorb_sound_w },
MEMORY_END


static MEMORY_READ16_START( warriorb_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x213fff, MRA16_RAM },	/* main ram */
	{ 0x300000, 0x313fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_r },
	{ 0x340000, 0x353fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x360000, 0x36000f, TC0100SCN_ctrl_word_1_r },
	{ 0x400000, 0x400007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MRA16_RAM },	/* sprite ram */
	{ 0x800000, 0x80000f, TC0510NIO_halfword_r },
	{ 0x830000, 0x830003, warriorb_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( warriorb_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x213fff, MWA16_RAM },
	{ 0x300000, 0x313fff, TC0100SCN_dual_screen_w },	/* tilemaps (all screens) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_w },
	{ 0x340000, 0x353fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x360000, 0x36000f, TC0100SCN_ctrl_word_1_w },
	{ 0x400000, 0x400007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x80000f, TC0510NIO_halfword_w },
/*	{ 0x820000, 0x820001, MWA16_NOP },	*/ /* ? uses bits 0,2,3*/
	{ 0x830000, 0x830003, warriorb_sound_w },
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( z80_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, warriorb_pancontrol }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/***********************************************************
                     INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_JAPAN_NEW_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

#define WARRIORB_SYSTEM_INPUT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \

#define WARRIORB_PLAYERS_INPUT_1 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

#define WARRIORB_PLAYERS_INPUT_2( freeze_button) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, freeze_button | IPF_PLAYER1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )


INPUT_PORTS_START( darius2d )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )  /* used, but manual in japanese*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Autofire" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "every 500k" )
	PORT_DIPSETTING(    0x0c, "every 700k" )
	PORT_DIPSETTING(    0x08, "every 800k" )
	PORT_DIPSETTING(    0x04, "every 900k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )  /* in manual*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	/* IN0 */
	WARRIORB_SYSTEM_INPUT
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	WARRIORB_PLAYERS_INPUT_1

	/* IN2 */
	WARRIORB_PLAYERS_INPUT_2( IPT_BUTTON3 )
INPUT_PORTS_END

INPUT_PORTS_START( warriorb )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, "Vitality Recovery" ) /*after finishing a level*/
	PORT_DIPSETTING(    0x02, "Less" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "More" )
	PORT_DIPSETTING(    0x00, "Most" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, "Gold Sheep at" )
	PORT_DIPSETTING(    0x04, "50k only" )
	PORT_DIPSETTING(    0x00, "50k and every 70k" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )	/*in manual*/
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Player Starting Strength" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Full" )
	PORT_DIPNAME( 0x20, 0x20, "Magician appears" )
	PORT_DIPSETTING(    0x20, "When you get a Crystal" )
	PORT_DIPSETTING(    0x00, "Always" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Rounds" )
	PORT_DIPSETTING(    0x80, "Normal (10-14, depends on skill)" )
	PORT_DIPSETTING(    0x00, "Long (14)" )

	/* IN0 */
	WARRIORB_SYSTEM_INPUT
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 | IPF_CHEAT )	/* Japanese version actually doesn't */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 | IPF_CHEAT )	/* have the third button */

	/* IN1 */
	WARRIORB_PLAYERS_INPUT_1

	/* IN2 */
	WARRIORB_PLAYERS_INPUT_2( IPT_BUTTON4 )
INPUT_PORTS_END


/***********************************************************
                        GFX DECODING
***********************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 11*4, 10*4, 1*4, 0*4, 9*4, 8*4, 7*4, 6*4, 15*4, 14*4, 5*4, 4*4, 13*4, 12*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo warriorb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles (screen 1) */
	{ REGION_GFX3, 0, &charlayout,  0, 256 },	/* scr tiles (screen 2) */
	{ -1 } /* end of array */
};


/**************************************************************
                           YM2610 (SOUND)
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};


/**************************************************************
                         SUBWOOFER (SOUND)
**************************************************************/

static int subwoofer_sh_start(const struct MachineSound *msound)
{
	/* Adjust the lowpass filter of the first three YM2610 channels */

	mixer_set_lowpass_frequency(0,20);
	mixer_set_lowpass_frequency(1,20);
	mixer_set_lowpass_frequency(2,20);

	return 0;
}

static struct CustomSound_interface subwoofer_interface =
{
	subwoofer_sh_start,
	0, /* none */
	0 /* none */
};


/***********************************************************
                       MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( darius2d )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? (Might well be 16!) */
	MDRV_CPU_MEMORY(darius2d_readmem,darius2d_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( taito_dualscreen )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(80*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 80*8-1, 3*8, 32*8-1)
	MDRV_GFXDECODE(warriorb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*2)

	MDRV_VIDEO_START(darius2d)
	MDRV_VIDEO_UPDATE(warriorb)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
	MDRV_SOUND_ADD(CUSTOM, subwoofer_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( warriorb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ? */
	MDRV_CPU_MEMORY(warriorb_readmem,warriorb_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( taito_dualscreen )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(80*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 80*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(warriorb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*2)

	MDRV_VIDEO_START(warriorb)
	MDRV_VIDEO_UPDATE(warriorb)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610B, ym2610_interface)
MACHINE_DRIVER_END


/***************************************************************************
                                 DRIVERS
***************************************************************************/

ROM_START( darius2d )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 512K for 68000 code */
	ROM_LOAD16_BYTE( "c07_20-2.74", 0x00000, 0x20000, CRC(a0f345b8) SHA1(1ce46e9707ec9ad51b26acf613eedc0536d227ae) )
	ROM_LOAD16_BYTE( "c07_19-2.73", 0x00001, 0x20000, CRC(925412c6) SHA1(7f1f62b7b2261c440dccd512ebd3faea141b7c83) )
	ROM_LOAD16_BYTE( "c07_21-2.76", 0x40000, 0x20000, CRC(bdd60e37) SHA1(777d3f67deba7df0da9d2605b2e2198f4bf47ebc) )
	ROM_LOAD16_BYTE( "c07_18-2.71", 0x40001, 0x20000, CRC(23fcd89b) SHA1(8aaf4ac836773d9b064ded68a6f092fe9eec7ac2) )

	ROM_LOAD16_WORD_SWAP( "c07-09.75",   0x80000, 0x80000, CRC(cc69c2ce) SHA1(47883b9e14d8b6dd74db221bff396477231938f2) )	/* data rom */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-17.69", 0x00000, 0x04000, CRC(ae16c905) SHA1(70ba5aacd8a8e00b94719e3955abad8827c67aa8) )
	ROM_CONTINUE(          0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03.12", 0x00000, 0x80000, CRC(189bafce) SHA1(d885e444523489fe24269b90dec58e0d92cfbd6e) )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04.11", 0x80000, 0x80000, CRC(50421e81) SHA1(27ac420602f1dac00dc32903543a518e6f47fb2f) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c07-06.27", 0x00000, 0x80000, CRC(5eebbcd6) SHA1(d4d860bf6b099956c45c7273ad77b1d35deba4c1) )	/* OBJ */
	ROM_LOAD32_BYTE( "c07-05.24", 0x00001, 0x80000, CRC(fb6d0550) SHA1(2d570ff5ef262cb4cb52e8584a7f167263194d37) )
	ROM_LOAD32_BYTE( "c07-08.25", 0x00002, 0x80000, CRC(a07dc846) SHA1(7199a604fcd693215ddb7670bfb2daf150145fd7) )
	ROM_LOAD32_BYTE( "c07-07.26", 0x00003, 0x80000, CRC(fd9f9e74) SHA1(e89beb5cac844fe16662465b0c76337692591aae) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screen 2) */

/* The actual board duplicates the SCR gfx roms for the 2nd TC0100SCN */
/*	ROM_LOAD( "c07-03.47", 0x00000, 0x80000, CRC(189bafce) SHA1(d885e444523489fe24269b90dec58e0d92cfbd6e) )*/
/*	ROM_LOAD( "c07-04.48", 0x80000, 0x80000, CRC(50421e81) SHA1(27ac420602f1dac00dc32903543a518e6f47fb2f) )*/

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10.95", 0x00000, 0x80000, CRC(4bbe0ed9) SHA1(081b73c4e4d4fa548445e5548573099bcb1e9213) )
	ROM_LOAD( "c07-11.96", 0x80000, 0x80000, CRC(3c815699) SHA1(0471ff5b0c0da905267f2cee52fd68c8661cccc9) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c07-12.107", 0x00000, 0x80000, CRC(e0b71258) SHA1(0258e308b643d723475824752ebffc4ea29d1ac4) )

	ROM_REGION( 0x001000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "c07-13.37", 0x00000, 0x00400, CRC(3ca18eb3) SHA1(54560f02c2be67993940831222130e90cd171991) )
	ROM_LOAD( "c07-14.38", 0x00000, 0x00400, CRC(baf2a193) SHA1(b7f103b5f5aab0702dd21fd7e3a82261ae1760e9) )

/* Pals, not dumped*/
/*	ROM_LOAD( "C07-15.78", 0x00000, 0x00?00, NO_DUMP )*/
/*	ROM_LOAD( "C07-16.79", 0x00000, 0x00?00, NO_DUMP )*/
ROM_END

ROM_START( drius2do )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 512K for 68000 code */
	ROM_LOAD16_BYTE( "c07_20-1.74", 0x00000, 0x20000, CRC(48b0804a) SHA1(932fb2cd55e6bfef84cf3cfaf3e75b4297a92b34) )
	ROM_LOAD16_BYTE( "c07_19-1.73", 0x00001, 0x20000, CRC(1f9a4f83) SHA1(d02caef350bdcac0ff771b5c92bb4e7435e0c9fa) )
	ROM_LOAD16_BYTE( "c07_21-1.76", 0x40000, 0x20000, CRC(b491b0ca) SHA1(dd7aa196c6002abc8e2f885f3f997f2279e59769) )
	ROM_LOAD16_BYTE( "c07_18-1.71", 0x40001, 0x20000, CRC(c552e42f) SHA1(dc952002a9a738cb1789f7c51acb71693ae03549) )

	ROM_LOAD16_WORD_SWAP( "c07-09.75",   0x80000, 0x80000, CRC(cc69c2ce) SHA1(47883b9e14d8b6dd74db221bff396477231938f2) )	/* data rom */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-17.69", 0x00000, 0x04000, CRC(ae16c905) SHA1(70ba5aacd8a8e00b94719e3955abad8827c67aa8) )
	ROM_CONTINUE(          0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03.12", 0x00000, 0x80000, CRC(189bafce) SHA1(d885e444523489fe24269b90dec58e0d92cfbd6e) )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04.11", 0x80000, 0x80000, CRC(50421e81) SHA1(27ac420602f1dac00dc32903543a518e6f47fb2f) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c07-06.27", 0x00000, 0x80000, CRC(5eebbcd6) SHA1(d4d860bf6b099956c45c7273ad77b1d35deba4c1) )	/* OBJ */
	ROM_LOAD32_BYTE( "c07-05.24", 0x00001, 0x80000, CRC(fb6d0550) SHA1(2d570ff5ef262cb4cb52e8584a7f167263194d37) )
	ROM_LOAD32_BYTE( "c07-08.25", 0x00002, 0x80000, CRC(a07dc846) SHA1(7199a604fcd693215ddb7670bfb2daf150145fd7) )
	ROM_LOAD32_BYTE( "c07-07.26", 0x00003, 0x80000, CRC(fd9f9e74) SHA1(e89beb5cac844fe16662465b0c76337692591aae) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screen 2) */

/* The actual board duplicates the SCR gfx roms for the 2nd TC0100SCN */
/*	ROM_LOAD( "c07-03.47", 0x00000, 0x80000, CRC(189bafce) SHA1(d885e444523489fe24269b90dec58e0d92cfbd6e) )*/
/*	ROM_LOAD( "c07-04.48", 0x80000, 0x80000, CRC(50421e81) SHA1(27ac420602f1dac00dc32903543a518e6f47fb2f) )*/

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10.95", 0x00000, 0x80000, CRC(4bbe0ed9) SHA1(081b73c4e4d4fa548445e5548573099bcb1e9213) )
	ROM_LOAD( "c07-11.96", 0x80000, 0x80000, CRC(3c815699) SHA1(0471ff5b0c0da905267f2cee52fd68c8661cccc9) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c07-12.107", 0x00000, 0x80000, CRC(e0b71258) SHA1(0258e308b643d723475824752ebffc4ea29d1ac4) )

	ROM_REGION( 0x001000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "c07-13.37", 0x00000, 0x00400, CRC(3ca18eb3) SHA1(54560f02c2be67993940831222130e90cd171991) )
	ROM_LOAD( "c07-14.38", 0x00000, 0x00400, CRC(baf2a193) SHA1(b7f103b5f5aab0702dd21fd7e3a82261ae1760e9) )
ROM_END

ROM_START( warriorb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 1024K for 68000 code */
	ROM_LOAD16_BYTE( "d24_20-1.74", 0x000000, 0x40000, CRC(4452dc25) SHA1(bbb4fbc25a3f263ce2716698cacaca201cb9591b) )
	ROM_LOAD16_BYTE( "d24_19-1.73", 0x000001, 0x40000, CRC(15c16016) SHA1(5b28834d8d5296c562c90a861c6ccdd46cc3c204) )
	ROM_LOAD16_BYTE( "d24_21-1.76", 0x080000, 0x40000, CRC(783ef8e1) SHA1(28a43d5231031b2ff3e437c3b6b8604f0d2b521b) )
	ROM_LOAD16_BYTE( "d24_18-1.71", 0x080001, 0x40000, CRC(4502db60) SHA1(b29c441ab79f753378ea47e7c22924db0cd5eb89) )

	ROM_LOAD16_WORD_SWAP( "d24-09.75",   0x100000, 0x100000, CRC(ece5cc59) SHA1(337db41d5a74fa4202b1be1a672a068ec3b205a8) )	/* data rom */
	/* Note: Raine wrongly doubles up d24-09 as delta-t samples */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "d24_17.69",  0x00000, 0x04000, CRC(e41e4aae) SHA1(9bf40b6e8aa5c6ec62c5d21edbb2214f6550c94f) )
	ROM_CONTINUE(           0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d24-02.12", 0x000000, 0x100000, CRC(9f50c271) SHA1(1a1b2ae7cb7785e7f66aa26258a6cd2921a29545) )	/* SCR A, screen 1 */
	ROM_LOAD( "d24-01.11", 0x100000, 0x100000, CRC(326dcca9) SHA1(1993776d71bca7d6dfc6f84dd9262d0dcae87f69) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "d24-06.27", 0x000000, 0x100000, CRC(918486fe) SHA1(cc9e287221ef33dba77a22975e23b250ba50b758) )	/* OBJ */
	ROM_LOAD32_BYTE( "d24-03.24", 0x000001, 0x100000, CRC(46db9fd7) SHA1(f08f3c9833d80ce161b06f4ae484c5c79539639c) )
	ROM_LOAD32_BYTE( "d24-04.25", 0x000002, 0x100000, CRC(148e0493) SHA1(f1cb819830e5bd544b11762784e228b5cb62b7e4) )
	ROM_LOAD32_BYTE( "d24-05.26", 0x000003, 0x100000, CRC(9f414317) SHA1(204cf47404e5e1085c1108abacd2b79a6cd0f74a) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "d24-07.47", 0x000000, 0x100000, CRC(9f50c271) SHA1(1a1b2ae7cb7785e7f66aa26258a6cd2921a29545) )	/* SCR B, screen 2 */
	ROM_LOAD( "d24-08.48", 0x100000, 0x100000, CRC(1e6d1528) SHA1(d6843aa67befd7db44f468be16ba2f0efb85d40f) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "d24-12.107", 0x000000, 0x100000, CRC(279203a1) SHA1(ed75e811a1f0863c134034457ce2e97372726bdb) )
	ROM_LOAD( "d24-10.95",  0x100000, 0x100000, CRC(0e0c716d) SHA1(5e2f334dd484678766c5a71196d9bad0ba0fe8d9) )
	ROM_LOAD( "d24-11.118", 0x200000, 0x100000, CRC(15362573) SHA1(8602c9f24134cac6fe1375fb189b152f0c68aeb7) )

	/* No Delta-T samples */

	ROM_REGION( 0x01000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "d24-13.37", 0x00000, 0x400, CRC(3ca18eb3) SHA1(54560f02c2be67993940831222130e90cd171991) )
	ROM_LOAD( "d24-14.38", 0x00000, 0x400, CRC(baf2a193) SHA1(b7f103b5f5aab0702dd21fd7e3a82261ae1760e9) )
/*	ROM_LOAD( "d24-15.78", 0x00000, 0xa??, NO_DUMP )	 // Pals /*/
/*	ROM_LOAD( "d24-16.79", 0x00000, 0xa??, NO_DUMP )*/
ROM_END


static DRIVER_INIT( warriorb )
{
	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}

MACHINE_INIT( taito_dualscreen )
{
	/**** mixer control enable ****/
	mixer_sound_enable_global_w( 1 );	/* mixer enabled */
}


/* Working Games */

GAME( 1989, darius2d, darius2,  darius2d, darius2d, warriorb, ROT0, "Taito Corporation", "Darius II (dual screen) (Japan)" )
GAME( 1989, drius2do, darius2,  darius2d, darius2d, warriorb, ROT0, "Taito Corporation", "Darius II (dual screen) (Japan old version)" )
GAME( 1991, warriorb, 0,        warriorb, warriorb, warriorb, ROT0, "Taito Corporation", "Warrior Blade - Rastan Saga Episode III (Japan)" )
