/***************************************************************************

Taito Triple Screen Games
=========================

Ninja Warriors (c) 1987 Taito
Darius 2       (c) 1989 Taito

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

The triple screen games operate on hardware with various similarities to
the Taito F2 system, as they share some custom ics e.g. the TC0100SCN.

According to Sixtoe: "The multi-monitor systems had 2 or 3 13" screens;
one in the middle facing the player, and the other 1 or 2 on either side
mounted below and facing directly up reflecting off special semi-reflecting
mirrors, with about 1" of the graphics being overlapped on each screen.
This was the only way to get uninterrupted screens and to be able to see
through both ways. Otherwise you`d have the monitors' edges visible.
You can tell if your arcade has been cheap (like one near me) when you
look at the screens and can see black triangles on the left or right, this
means they bought ordinary mirrors and you can't see through them the
wrong way, as the semi-reflecting mirrors were extremely expensive."

For each screen the games have 3 separate layers of graphics:- one
128x64 tiled scrolling background plane of 8x8 tiles, a similar
foreground plane, and a 128x32 text plane with character definitions
held in ram.

Writing to the first TC0100SCN "writes through" to the two subsidiary
chips so that all three have identical contents. The subsidiary ones are
only addressed individually during initial memory checks, I think. (?)

There is a single sprite plane which covers all 3 screens.
The sprites are 16x16 and are not zoomable.

Twin 68000 processors are used; both have access to sprite ram and
the tilemap areas, and they communicate via 64K of shared ram.

Sound is dealt with by a Z80 controlling a YM2610. Sound commands
are written to the Z80 by the 68000 (the same as in Taito F2 games).


Tilemaps
========

TC0100SCN has tilemaps twice as wide as usual. The two BG tilemaps take
up twice the usual space, $8000 bytes each. The text tilemap takes up
the usual space, because its height is halved.

The triple palette generator (one for each screen) is probably just a
result of the way the hardware works: the colors in each are the same.


Dumpers Notes
-------------

Ninja Warriors (JPN Ver.)
(c)1987 Taito

Sound Board
K1100313A
CPU 	:Z80
Sound	:YM2610
OSC 	:16000.00KHz
Other	:TC0140SYT,TC0060DCA x2
-----------------------
B31-08.19
B31-09.18
B31-10.17
B31-11.16
B31_37.11
-----------------------
CPU Board
M4300086A
K1100311A
CPU 	:TS68000CP8 x2
Sound	:YM2610
OSC 	:26686.00KHz,16000.00KHz
Other	:TC0040IOC,TC0070RGB x3,TC0110PCR x3,TC0100SCN x3
-----------------------
B31-01.23
B31-01.26
B31-01.28
B31-02.24
B31-02.27
B31-02.29
B31_27.31
B31_28.32
B31_29.34
B31_30.35
B31_31.85
B31_32.86
B31_33.87
B31_34.95
B31_35.96
B31_36.97
B31_38.3
B31_39.2
B31_40.6
B31_41.5
-----------------------
OBJECT Board
K1100312A
Other	:TC0120SHT
-----------------------
B31-04.173
B31-05.174
B31-06.175
B31-07.176
B31-25.38
B31-26.58


TODO
====

Verify 68000 clock rates. Unknown sprite bits.


Ninjaw
------

"Subwoofer" sound filtering isn't perfect.

Some enemies slide relative to the background when they should
be standing still. High cpu interleaving doesn't help much.


Darius 2
--------

"Subwoofer" sound filtering isn't perfect.

(When you lose a life or big enemies appear it's meant to create
rumbling on a subwoofer in the cabinet.)


***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

MACHINE_INIT( ninjaw );

VIDEO_START( ninjaw );
VIDEO_UPDATE( ninjaw );

static UINT16 cpua_ctrl = 0xff;

static size_t sharedram_size;
static data16_t *sharedram;


static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static void parse_control(void)	/* assumes Z80 sandwiched between 68Ks */
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
	   if cpu B is disabled !! */
	cpu_set_reset_line(2,(cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

}

static WRITE16_HANDLER( cpua_ctrl_w )
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control();

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: write %04x to cpu control\n",activecpu_get_pc(),data);
}


/*****************************************
			SOUND
*****************************************/

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

static WRITE16_HANDLER( ninjaw_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff00)
		usrintf_showmessage("ninjaw_sound_w to high byte: %04x",data);
#endif
}

static READ16_HANDLER( ninjaw_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


/**** sound pan control ****/
static int ninjaw_pandata[4];
WRITE_HANDLER( ninjaw_pancontrol )
{
  offset = offset&3;
  ninjaw_pandata[offset] = (float)data * (100.f / 255.0f);
  /*usrintf_showmessage(" pan %02x %02x %02x %02x", ninjaw_pandata[0], ninjaw_pandata[1], ninjaw_pandata[2], ninjaw_pandata[3] );*/
  if( offset < 2 ){
    mixer_set_stereo_volume( 3, ninjaw_pandata[0], ninjaw_pandata[1] );
  }
  else{
    mixer_set_stereo_volume( 4, ninjaw_pandata[2], ninjaw_pandata[3] );
  }
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( ninjaw_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x220000, 0x220003, ninjaw_sound_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, MRA16_RAM },			/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_r },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_r },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_r },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_r },
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( ninjaw_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x210000, 0x210001, cpua_ctrl_w },
	{ 0x220000, 0x220003, ninjaw_sound_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x260000, 0x263fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x280000, 0x293fff, TC0100SCN_triple_screen_w },	/* tilemaps (all screens) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_w },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_w },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_w },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_w },
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END

/* NB there could be conflicts between which cpu writes what to the*/
/* palette, as our interleaving won't match the original board.*/

static MEMORY_READ16_START( ninjaw_cpub_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, spriteram16_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( ninjaw_cpub_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram },
	{ 0x260000, 0x263fff, spriteram16_w },
	{ 0x280000, 0x293fff, TC0100SCN_triple_screen_w },	/* tilemaps (all screens) */
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END


static MEMORY_READ16_START( darius2_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x220000, 0x220003, ninjaw_sound_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, MRA16_RAM },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_r },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_r },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_r },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_r },
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( darius2_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x210000, 0x210001, cpua_ctrl_w },
	{ 0x220000, 0x220003, ninjaw_sound_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x260000, 0x263fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x280000, 0x293fff, TC0100SCN_triple_screen_w },	/* tilemaps (all screens) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_w },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_w },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_w },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_w },
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_READ16_START( darius2_cpub_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, spriteram16_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
MEMORY_END

static MEMORY_WRITE16_START( darius2_cpub_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram },
	{ 0x260000, 0x263fff, spriteram16_w },
	{ 0x280000, 0x293fff, TC0100SCN_triple_screen_w },	/* tilemaps (all screens) */
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
	{ 0xe400, 0xe403, ninjaw_pancontrol }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define NINJAW_DSWA \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

#define NINJAW_DSWB \
	PORT_START \
	TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )  /* all 6 in manual */ \
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On) ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On) ) \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On) ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On) ) \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

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

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

#define NINJAW_IN2 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Stops working if this is high */ \
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	/* Freezes game */ \
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

#define NINJAW_IN3 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

#define NINJAW_IN4 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

INPUT_PORTS_START( ninjaw )
	NINJAW_DSWA
	TAITO_COINAGE_WORLD_8

	NINJAW_DSWB

	NINJAW_IN2

	NINJAW_IN3

	NINJAW_IN4
INPUT_PORTS_END

INPUT_PORTS_START( ninjawj )
	NINJAW_DSWA
	TAITO_COINAGE_JAPAN_8

	NINJAW_DSWB

	NINJAW_IN2

	NINJAW_IN3

	NINJAW_IN4
INPUT_PORTS_END

INPUT_PORTS_START( darius2 )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Continuous Fire" )
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
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	NINJAW_IN2

	NINJAW_IN3

	NINJAW_IN4
INPUT_PORTS_END


/***********************************************************
				GFX DECODING

	(Thanks to Raine for the obj decoding)
***********************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },	/* pixel bits separated, jump 4 to get to next one */
	{ 3, 2, 1, 0, 19, 18, 17, 16,
	  3+ 32*8, 2+ 32*8, 1+ 32*8, 0+ 32*8, 19+ 32*8, 18+ 32*8, 17+ 32*8, 16+ 32*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  64*8 + 0*32, 64*8 + 1*32, 64*8 + 2*32, 64*8 + 3*32,
	  64*8 + 4*32, 64*8 + 5*32, 64*8 + 6*32, 64*8 + 7*32 },
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

static struct GfxDecodeInfo ninjaw_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles (screen 1) */
	{ REGION_GFX3, 0, &charlayout,  0, 256 },	/* scr tiles (screens 2+) */
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

	/* The 150 Hz is a common top frequency played by a generic */
	/* subwoofer, the real Arcade Machine may differs */

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


/*************************************************************
			     MACHINE DRIVERS

Ninjaw: high interleaving of 100, but doesn't stop enemies
"sliding" when they should be standing still relative
to the scrolling background.

Darius2: arbitrary interleaving of 10 to keep cpus synced.
*************************************************************/

static MACHINE_DRIVER_START( ninjaw )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_MEMORY(ninjaw_readmem,ninjaw_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 16/4 MHz ? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_MEMORY(ninjaw_cpub_readmem,ninjaw_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* CPU slices */

	MDRV_MACHINE_INIT(ninjaw)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(12,3)
	MDRV_SCREEN_SIZE(110*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 108*8-1, 3*8, 31*8-1)
	MDRV_GFXDECODE(ninjaw_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*3)

	MDRV_VIDEO_START(ninjaw)
	MDRV_VIDEO_UPDATE(ninjaw)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
	MDRV_SOUND_ADD(CUSTOM, subwoofer_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( darius2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_MEMORY(darius2_readmem,darius2_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_MEMORY(darius2_cpub_readmem,darius2_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* CPU slices */

	MDRV_MACHINE_INIT(ninjaw)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(12,3)
	MDRV_SCREEN_SIZE(110*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 108*8-1, 3*8, 31*8-1)
	MDRV_GFXDECODE(ninjaw_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*3)

	MDRV_VIDEO_START(ninjaw)
	MDRV_VIDEO_UPDATE(ninjaw)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
	MDRV_SOUND_ADD(CUSTOM, subwoofer_interface)
MACHINE_DRIVER_END


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( ninjaw )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 256K for 68000 CPUA code */
	ROM_LOAD16_BYTE( "b31-45",    0x00000, 0x10000, CRC(107902c3) SHA1(026f71a918059e3374ae262304a2ee1270f5c5bd) )
	ROM_LOAD16_BYTE( "b31-47",    0x00001, 0x10000, CRC(bd536b1e) SHA1(39c86cbb3a33fc77a0141b5648a1aca862e0a5fd) )
	ROM_LOAD16_BYTE( "b31_29.34", 0x20000, 0x10000, CRC(f2941a37) SHA1(cf1f231d9caddc903116a8b654f49181ca459697) )
	ROM_LOAD16_BYTE( "b31_27.31", 0x20001, 0x10000, CRC(2f3ff642) SHA1(7d6775b51d96b459b163d8fde2385b0e3f5242ca) )

	ROM_LOAD16_BYTE( "b31_41.5", 0x40000, 0x20000, CRC(0daef28a) SHA1(7c7e16b0eebc589ab99f62ddb98b372596ff5ae6) )	/* data roms ? */
	ROM_LOAD16_BYTE( "b31_39.2", 0x40001, 0x20000, CRC(e9197c3c) SHA1(a7f0ef2b3c4258c09edf05284fec45832a8fb147) )
	ROM_LOAD16_BYTE( "b31_40.6", 0x80000, 0x20000, CRC(2ce0f24e) SHA1(39632397ac7e8457607c32c31fccf1c08d4b2621) )
	ROM_LOAD16_BYTE( "b31_38.3", 0x80001, 0x20000, CRC(bc68cd99) SHA1(bb31ea589339c9f9b61e312e1024b5c8410cdb43) )

	ROM_REGION( 0x60000, REGION_CPU3, 0 )	/* 384K for 68000 CPUB code */
	ROM_LOAD16_BYTE( "b31_33.87", 0x00000, 0x10000, CRC(6ce9af44) SHA1(486e332af238c211c3f64f7ead114282661687c4) )
	ROM_LOAD16_BYTE( "b31_36.97", 0x00001, 0x10000, CRC(ba20b0d4) SHA1(fb3dcb7681a95087afac9aa9393765d786243486) )
	ROM_LOAD16_BYTE( "b31_32.86", 0x20000, 0x10000, CRC(e6025fec) SHA1(071f83a9ddebe67bd6c6c2505318e177895163ee) )
	ROM_LOAD16_BYTE( "b31_35.96", 0x20001, 0x10000, CRC(70d9a89f) SHA1(20f846beb052fd8cddcf00c3e42e3304e102a87b) )
	ROM_LOAD16_BYTE( "b31_31.85", 0x40000, 0x10000, CRC(837f47e2) SHA1(88d596f01566456ba18a01afd0a6a7c121d3ca88) )
	ROM_LOAD16_BYTE( "b31_34.95", 0x40001, 0x10000, CRC(d6b5fb2a) SHA1(e3ae0d7ec62740465a90e4939b10341d3866d860) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b31_37.11",  0x00000, 0x04000, CRC(0ca5799d) SHA1(6485dde076d15b69b9ee65880dda57ad4f8d129c) )
	ROM_CONTINUE(           0x10000, 0x1c000 )  /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-01.23", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	/* SCR (screen 1) */
	ROM_LOAD( "b31-02.24", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-07.176", 0x000000, 0x80000, CRC(33568cdb) SHA1(87abf56bbbd3659a1bd3e6ce9e43176be7950b41) )	/* OBJ */
	ROM_LOAD( "b31-06.175", 0x080000, 0x80000, CRC(0d59439e) SHA1(54d844492888e7fe2c3bc61afe64f8d47fdee8dc) )
	ROM_LOAD( "b31-05.174", 0x100000, 0x80000, CRC(0a1fc9fb) SHA1(a5d6975fd4f7e689c8cafd7c9cd3787797955779) )
	ROM_LOAD( "b31-04.173", 0x180000, 0x80000, CRC(2e1e4cb5) SHA1(4733cfc015a68e021108a9e1e8ea807b0e7eac7a) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screens 2+) */

/* The actual board duplicates the SCR gfx roms for 2nd/3rd TC0100SCN */
/*	ROM_LOAD( "b31-01.26", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	 // SCR (screen 2) /*/
/*	ROM_LOAD( "b31-02.27", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )*/
/*	ROM_LOAD( "b31-01.28", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	 // SCR (screen 3) /*/
/*	ROM_LOAD( "b31-02.29", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )*/

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b31-09.18", 0x000000, 0x80000, CRC(60a73382) SHA1(0ddeb86fcd4d19a58e62bf8564f996d17e36e5c5) )
	ROM_LOAD( "b31-10.17", 0x080000, 0x80000, CRC(c6434aef) SHA1(3348ce87882e3f668aa85bbb517975ec1fc9b6fd) )
	ROM_LOAD( "b31-11.16", 0x100000, 0x80000, CRC(8da531d4) SHA1(525dfab0a0729e9fb6f0e4c8187bf4ce16321b20) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b31-08.19", 0x000000, 0x80000, CRC(a0a1f87d) SHA1(6b0f8094f3a3ef1ced76984e333e22a17c51af29) )

	ROM_REGION( 0x01000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "b31-25.38", 0x00000, 0x200, CRC(a0b4ba48) SHA1(dc9a46366a0cbf63a609f177c3d3ba9675416662) )
	ROM_LOAD( "b31-26.58", 0x00000, 0x200, CRC(13e5fe15) SHA1(c973c7965954a2a0b427908f099592ed89cf0ff0) )
ROM_END

ROM_START( ninjawj )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 256K for 68000 CPUA code */
	ROM_LOAD16_BYTE( "b31_30.35", 0x00000, 0x10000, CRC(056edd9f) SHA1(8922cede80b31ce0f7a00c8cab13d835464c6058) )
	ROM_LOAD16_BYTE( "b31_28.32", 0x00001, 0x10000, CRC(cfa7661c) SHA1(a7a6abb33a514d910e3198d5acbd4c31b2434b6c) )
	ROM_LOAD16_BYTE( "b31_29.34", 0x20000, 0x10000, CRC(f2941a37) SHA1(cf1f231d9caddc903116a8b654f49181ca459697) )
	ROM_LOAD16_BYTE( "b31_27.31", 0x20001, 0x10000, CRC(2f3ff642) SHA1(7d6775b51d96b459b163d8fde2385b0e3f5242ca) )

	ROM_LOAD16_BYTE( "b31_41.5", 0x40000, 0x20000, CRC(0daef28a) SHA1(7c7e16b0eebc589ab99f62ddb98b372596ff5ae6) )	/* data roms ? */
	ROM_LOAD16_BYTE( "b31_39.2", 0x40001, 0x20000, CRC(e9197c3c) SHA1(a7f0ef2b3c4258c09edf05284fec45832a8fb147) )
	ROM_LOAD16_BYTE( "b31_40.6", 0x80000, 0x20000, CRC(2ce0f24e) SHA1(39632397ac7e8457607c32c31fccf1c08d4b2621) )
	ROM_LOAD16_BYTE( "b31_38.3", 0x80001, 0x20000, CRC(bc68cd99) SHA1(bb31ea589339c9f9b61e312e1024b5c8410cdb43) )

	ROM_REGION( 0x60000, REGION_CPU3, 0 )	/* 384K for 68000 CPUB code */
	ROM_LOAD16_BYTE( "b31_33.87", 0x00000, 0x10000, CRC(6ce9af44) SHA1(486e332af238c211c3f64f7ead114282661687c4) )
	ROM_LOAD16_BYTE( "b31_36.97", 0x00001, 0x10000, CRC(ba20b0d4) SHA1(fb3dcb7681a95087afac9aa9393765d786243486) )
	ROM_LOAD16_BYTE( "b31_32.86", 0x20000, 0x10000, CRC(e6025fec) SHA1(071f83a9ddebe67bd6c6c2505318e177895163ee) )
	ROM_LOAD16_BYTE( "b31_35.96", 0x20001, 0x10000, CRC(70d9a89f) SHA1(20f846beb052fd8cddcf00c3e42e3304e102a87b) )
	ROM_LOAD16_BYTE( "b31_31.85", 0x40000, 0x10000, CRC(837f47e2) SHA1(88d596f01566456ba18a01afd0a6a7c121d3ca88) )
	ROM_LOAD16_BYTE( "b31_34.95", 0x40001, 0x10000, CRC(d6b5fb2a) SHA1(e3ae0d7ec62740465a90e4939b10341d3866d860) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b31_37.11",  0x00000, 0x04000, CRC(0ca5799d) SHA1(6485dde076d15b69b9ee65880dda57ad4f8d129c) )
	ROM_CONTINUE(           0x10000, 0x1c000 )  /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-01.23", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	/* SCR (screen 1) */
	ROM_LOAD( "b31-02.24", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-07.176", 0x000000, 0x80000, CRC(33568cdb) SHA1(87abf56bbbd3659a1bd3e6ce9e43176be7950b41) )	/* OBJ */
	ROM_LOAD( "b31-06.175", 0x080000, 0x80000, CRC(0d59439e) SHA1(54d844492888e7fe2c3bc61afe64f8d47fdee8dc) )
	ROM_LOAD( "b31-05.174", 0x100000, 0x80000, CRC(0a1fc9fb) SHA1(a5d6975fd4f7e689c8cafd7c9cd3787797955779) )
	ROM_LOAD( "b31-04.173", 0x180000, 0x80000, CRC(2e1e4cb5) SHA1(4733cfc015a68e021108a9e1e8ea807b0e7eac7a) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screens 2+) */

/* The actual board duplicates the SCR gfx roms for 2nd/3rd TC0100SCN */
/*	ROM_LOAD( "b31-01.26", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	 // SCR (screen 2) /*/
/*	ROM_LOAD( "b31-02.27", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )*/
/*	ROM_LOAD( "b31-01.28", 0x00000, 0x80000, CRC(8e8237a7) SHA1(3e181a153d9b4b7f6a620614ea9022285583a5b5) )	 // SCR (screen 3) /*/
/*	ROM_LOAD( "b31-02.29", 0x80000, 0x80000, CRC(4c3b4e33) SHA1(f99b379be1af085bf102d4d7cf35803e002fe80b) )*/

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b31-09.18", 0x000000, 0x80000, CRC(60a73382) SHA1(0ddeb86fcd4d19a58e62bf8564f996d17e36e5c5) )
	ROM_LOAD( "b31-10.17", 0x080000, 0x80000, CRC(c6434aef) SHA1(3348ce87882e3f668aa85bbb517975ec1fc9b6fd) )
	ROM_LOAD( "b31-11.16", 0x100000, 0x80000, CRC(8da531d4) SHA1(525dfab0a0729e9fb6f0e4c8187bf4ce16321b20) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b31-08.19", 0x000000, 0x80000, CRC(a0a1f87d) SHA1(6b0f8094f3a3ef1ced76984e333e22a17c51af29) )

	ROM_REGION( 0x01000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "b31-25.38", 0x00000, 0x200, CRC(a0b4ba48) SHA1(dc9a46366a0cbf63a609f177c3d3ba9675416662) )
	ROM_LOAD( "b31-26.58", 0x00000, 0x200, CRC(13e5fe15) SHA1(c973c7965954a2a0b427908f099592ed89cf0ff0) )
ROM_END

ROM_START( darius2 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 256K for 68000 CPUA code */
	ROM_LOAD16_BYTE( "c07-32-1", 0x00000, 0x10000, CRC(216c8f6a) SHA1(493b0779b99a228911f56ef9d2d4a3945683bec0) )
	ROM_LOAD16_BYTE( "c07-29-1", 0x00001, 0x10000, CRC(48de567f) SHA1(cdf50052933cd2603fd4374e8bae8b30a6c690b5) )
	ROM_LOAD16_BYTE( "c07-31-1", 0x20000, 0x10000, CRC(8279d2f8) SHA1(bd3c80a024a58e4b554f4867f56d7f5741eb3031) )
	ROM_LOAD16_BYTE( "c07-30-1", 0x20001, 0x10000, CRC(6122e400) SHA1(2f68a423f9db8d69ab74453f8cef755f703cc94c) )

	ROM_LOAD16_BYTE( "c07-27",   0x40000, 0x20000, CRC(0a6f7b6c) SHA1(0ed915201fbc0bf94fdcbef8dfd021cebe87474f) )	/* data roms ? */
	ROM_LOAD16_BYTE( "c07-25",   0x40001, 0x20000, CRC(059f40ce) SHA1(b05a96580edb66221af2f222df74a020366ce3ea) )
	ROM_LOAD16_BYTE( "c07-26",   0x80000, 0x20000, CRC(1f411242) SHA1(0fca5d864c1925473d0058e4cf81ad926f56cb14) )
	ROM_LOAD16_BYTE( "c07-24",   0x80001, 0x20000, CRC(486c9c20) SHA1(9e98fcc1777f044d69cc93eda674501b3be26097) )

	ROM_REGION( 0x60000, REGION_CPU3, 0 )	/* 384K for 68000 CPUB code */
	ROM_LOAD16_BYTE( "c07-35-1", 0x00000, 0x10000, CRC(dd8c4723) SHA1(e17159f894ee661a84ccd53e2d00ee78f2b46196) )
	ROM_LOAD16_BYTE( "c07-38-1", 0x00001, 0x10000, CRC(46afb85c) SHA1(a08fb9fd2bf0929a5599ab015680fa663f1d4fe6) )
	ROM_LOAD16_BYTE( "c07-34-1", 0x20000, 0x10000, CRC(296984b8) SHA1(3ba28e293c9d3ce01ee2f8ae2c2aa450fe021d30) )
	ROM_LOAD16_BYTE( "c07-37-1", 0x20001, 0x10000, CRC(8b7d461f) SHA1(c783491ca23223dc58fa7e8f408407b9a10cbce4) )
	ROM_LOAD16_BYTE( "c07-33-1", 0x40000, 0x10000, CRC(2da03a3f) SHA1(f1f2de82e0addc5e19c8935e4f5810896691118f) )
	ROM_LOAD16_BYTE( "c07-36-1", 0x40001, 0x10000, CRC(02cf2b1c) SHA1(c94a64f26f94f182cfe2b6edb37e4ce35a0f681b) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-28",  0x00000, 0x04000, CRC(da304bc5) SHA1(689b4f329d9a640145f82e12dff3dd1fcf8a28c8) )
	ROM_CONTINUE(            0x10000, 0x1c000 )  /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03.12", 0x00000, 0x80000, CRC(189bafce) SHA1(d885e444523489fe24269b90dec58e0d92cfbd6e) )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04.11", 0x80000, 0x80000, CRC(50421e81) SHA1(27ac420602f1dac00dc32903543a518e6f47fb2f) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-01", 0x00000, 0x80000, CRC(3cf0f050) SHA1(f5a1f7e327a2617fb95ce2837e72945fd7447346) )	/* OBJ */
	ROM_LOAD( "c07-02", 0x80000, 0x80000, CRC(75d16d4b) SHA1(795423278b66eca41accce1f8a4425d65af7b629) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screens 2+) */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10.95", 0x00000, 0x80000, CRC(4bbe0ed9) SHA1(081b73c4e4d4fa548445e5548573099bcb1e9213) )
	ROM_LOAD( "c07-11.96", 0x80000, 0x80000, CRC(3c815699) SHA1(0471ff5b0c0da905267f2cee52fd68c8661cccc9) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c07-12.107", 0x00000, 0x80000, CRC(e0b71258) SHA1(0258e308b643d723475824752ebffc4ea29d1ac4) )
ROM_END


static DRIVER_INIT( ninjaw )
{
	cpua_ctrl = 0xff;
	state_save_register_UINT16("main1", 0, "control", &cpua_ctrl, 1);
	state_save_register_func_postload(parse_control);

	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}

MACHINE_INIT( ninjaw )
{
  /**** mixer control enable ****/
  mixer_sound_enable_global_w( 1 );	/* mixer enabled */
}


/* Working Games */

GAME( 1987, ninjaw,   0,      ninjaw,  ninjaw,   ninjaw,  ROT0, "Taito Corporation Japan", "The Ninja Warriors (World)" )
GAME( 1987, ninjawj,  ninjaw, ninjaw,  ninjawj,  ninjaw,  ROT0, "Taito Corporation", "The Ninja Warriors (Japan)" )
GAME( 1989, darius2,  0,      darius2, darius2,  ninjaw,  ROT0, "Taito Corporation", "Darius II (Japan)" )
