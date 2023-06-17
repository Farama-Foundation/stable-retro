/***************************************************************************

Taito H system
----------------------------
driver by Yochizo

This driver is heavily dependent on the Raine source.
Very thanks to Richard Bush and the Raine team. Also,
I have been given a lot of helpful informations by
Yasuhiro Ogawa. Thank you, Yasu.


Supported games :
==================
 Syvalion          (C) 1988 Taito
 Record Breaker    (C) 1988 Taito
 Dynamite League   (C) 1990 Taito


System specs :
===============
 CPU   : MC68000 (12 MHz) x 1, Z80 (4 MHz?, sound CPU) x 1
 Sound : YM2610, YM3016?
 OSC   : 20.000 MHz, 8.000 MHz, 24.000 MHz
 Chips : TC0070RGB (Palette?)
         TC0220IOC (Input)
         TC0140SYT (Sound communication)
         TC0130LNB (???)
         TC0160ROM (???)
         TC0080VCO (Video?)

 name             irq    resolution   tx-layer   tc0220ioc
 --------------------------------------------------------------
 Syvalion          2       512x400      used     port-based
 Record Breaker    2       320x240     unused    port-based
 Dynamite League   1       320x240     unused   address-based


Known issues :
===============
 - Y coordinate of sprite zooming is non-linear, so currently implemented
   hand-tuned value and this is used for only Record Breaker.
 - Sprite and BG1 priority bit is not understood. It is defined by sprite
   priority in Record Breaker and by zoom value and some scroll value in
   Dynamite League. So, some priority problems still remain.
 - A scroll value of text layer seems to be nonexistence.
 - Background zoom effect is not working in flip screen mode.
 - Sprite zoom is a bit wrong.
 - Title screen of dynamite league is wrong a bit, which is mentioned by
   Yasuhiro Ogawa.

TODO :
========
 - Make dipswitches clear.
 - Implemented BG1 : sprite priority. Currently it is not brought out priority
   bit.
 - Fix sprite coordinates.
 - Improve zoom y coordinate.
 - Text layer scroll if exists.
 - Speed up and clean up the code [some of video routines now in taitoic.c]

Dleague: junky sprites (sometimes) at bottom of screen in
flipscreen.

Recordbr: missing hand of opponent when he ends in swimming
race and you're still on the blocks. Bug?

Recordbr: loads of unmapped IOC reads and writes.

****************************************************************************/


#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"
#include "vidhrdw/taitoic.h"



/***************************************************************************

  Variable

***************************************************************************/

static data16_t *taitoh_68000_mainram;

VIDEO_START( syvalion );
VIDEO_START( recordbr );
VIDEO_START( dleague );
VIDEO_UPDATE( syvalion );
VIDEO_UPDATE( recordbr );
VIDEO_UPDATE( dleague );


/***************************************************************************

  Interrupt(s)

***************************************************************************/

/* Handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface syvalion_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2610interface dleague_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static READ16_HANDLER( taitoh_mirrorram_r )
{
	/* This is a read handler of main RAM mirror. */
	return taitoh_68000_mainram[offset];
}

static READ16_HANDLER( syvalion_input_bypass_r )
{
	/* Bypass TC0220IOC controller for analog input */

	data8_t	port = TC0220IOC_port_r(0);	/* read port number */

	switch( port )
	{
		case 0x08:				/* trackball y coords bottom 8 bits for 2nd player */
			return input_port_7_r(0);

		case 0x09:				/* trackball y coords top 8 bits for 2nd player */
			if (input_port_7_r(0) & 0x80)	/* y- direction (negative value) */
				return 0xff;
			else							/* y+ direction (positive value) */
				return 0x00;

		case 0x0a:				/* trackball x coords bottom 8 bits for 2nd player */
			return input_port_6_r(0);

		case 0x0b:				/* trackball x coords top 8 bits for 2nd player */
			if (input_port_6_r(0) & 0x80)	/* x- direction (negative value) */
				return 0xff;
			else							/* x+ direction (positive value) */
				return 0x00;

		case 0x0c:				/* trackball y coords bottom 8 bits for 2nd player */
			return input_port_5_r(0);

		case 0x0d:				/* trackball y coords top 8 bits for 1st player */
			if (input_port_5_r(0) & 0x80)	/* y- direction (negative value) */
				return 0xff;
			else							/* y+ direction (positive value) */
				return 0x00;

		case 0x0e:				/* trackball x coords bottom 8 bits for 1st player */
			return input_port_4_r(0);

		case 0x0f:				/* trackball x coords top 8 bits for 1st player */
			if (input_port_4_r(0) & 0x80)	/* x- direction (negative value) */
				return 0xff;
			else							/* x+ direction (positive value) */
				return 0x00;

		default:
			return TC0220IOC_portreg_r( offset );
	}
}


static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank(1, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 3;
	reset_sound_region();
}


/***************************************************************************

  Memory Map(s)

***************************************************************************/

static MEMORY_READ16_START( syvalion_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x200001, syvalion_input_bypass_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x420fff, TC0080VCO_word_r },
	{ 0x500800, 0x500fff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( syvalion_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x420fff, TC0080VCO_word_w },
	{ 0x500800, 0x500fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( recordbr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x420fff, TC0080VCO_word_r },
	{ 0x500800, 0x500fff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( recordbr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x420fff, TC0080VCO_word_w },
	{ 0x500800, 0x500fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( dleague_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x420fff, TC0080VCO_word_r },
	{ 0x500800, 0x500fff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( dleague_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x420fff, TC0080VCO_word_w },
	{ 0x500800, 0x500fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x600000, 0x600001, MWA16_NOP },	/* ?? writes zero once per frame */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP },		/* pan control */
	{ 0xee00, 0xee00, MWA_NOP }, 		/* ? */
	{ 0xf000, 0xf000, MWA_NOP }, 		/* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

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

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( syvalion )
	PORT_START  /* DSW1 (0) */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSW2 (1) */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000k only" )
	PORT_DIPSETTING(    0x0c, "1500k only" )
	PORT_DIPSETTING(    0x04, "2000k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* TRACKBALL1 X (4) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_CENTER, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL1 Y (5) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1 | IPF_CENTER | IPF_REVERSE, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL2 X (6) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_CENTER, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL2 Y (7) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 30, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( recordbr )
	PORT_START  /* DSW1 (0) */
	PORT_DIPNAME( 0x00, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSW2 (1) */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 (4) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( dleague )
	PORT_START  /* DSW1 (0) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSW2 (1) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Innings per credit" )
	PORT_DIPSETTING(    0x0c, "3-3-3" )
	PORT_DIPSETTING(    0x08, "6-3" )
	PORT_DIPSETTING(    0x04, "3-2-2-2" )
	PORT_DIPSETTING(    0x00, "5-2-2" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 (4) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16x16 pixels */
	32768,	/* 32768 tiles */
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 0x100000*8+4, 0x100000*8, 0x100000*8+12, 0x100000*8+8,
	    0x200000*8+4, 0x200000*8, 0x200000*8+12, 0x200000*8+8, 0x300000*8+4, 0x300000*8, 0x300000*8+12, 0x300000*8+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxLayout charlayout =
{
	8, 8,	/* 8x8 pixels */
	256,	/* 256 chars */
	4,		/* 4 bit per pixel */
	{ 0x1000*8 + 8, 0x1000*8, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8
};


static struct GfxDecodeInfo syvalion_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo recordbr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo dleague_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ REGION_GFX2, 0, &charlayout, 32*16, 16    },	/* seems to be bogus...?*/
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( syvalion )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000 / 2)		/* 12 MHz */
	MDRV_CPU_MEMORY(syvalion_readmem,syvalion_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000 / 2)		/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 64*16)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 3*16, 28*16-1)
	MDRV_GFXDECODE(syvalion_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33*16)

	MDRV_VIDEO_START(syvalion)
	MDRV_VIDEO_UPDATE(syvalion)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, syvalion_ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( recordbr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000 / 2)		/* 12 MHz */
	MDRV_CPU_MEMORY(recordbr_readmem,recordbr_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000 / 2)		/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 64*16)
	MDRV_VISIBLE_AREA(1*16, 21*16-1, 2*16, 17*16-1)
	MDRV_GFXDECODE(recordbr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32*16)

	MDRV_VIDEO_START(recordbr)
	MDRV_VIDEO_UPDATE(recordbr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, syvalion_ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dleague )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000 / 2)		/* 12 MHz */
	MDRV_CPU_MEMORY(dleague_readmem,dleague_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000 / 2)		/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 64*16)
	MDRV_VISIBLE_AREA(1*16, 21*16-1, 2*16, 17*16-1)
	MDRV_GFXDECODE(dleague_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33*16)

	MDRV_VIDEO_START(dleague)
	MDRV_VIDEO_UPDATE(dleague)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, dleague_ym2610_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( syvalion )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* main cpu */
	ROM_LOAD16_BYTE( "b51-20.bin", 0x00000, 0x20000, CRC(440b6418) SHA1(262b65f39eb13c11ae7b87013951097ab0a9cb63) )
	ROM_LOAD16_BYTE( "b51-22.bin", 0x00001, 0x20000, CRC(e6c61079) SHA1(b786ef1bfc72706347c12c17616652bc8302a98c) )
	ROM_LOAD16_BYTE( "b51-19.bin", 0x40000, 0x20000, CRC(2abd762c) SHA1(97cdb9f1dba5b11b96b5d3431937669de5220512) )
	ROM_LOAD16_BYTE( "b51-21.bin", 0x40001, 0x20000, CRC(aa111f30) SHA1(77da4a8db49999f5fa2cf0209028d0f70e26dfe3) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )		/* sound cpu */
	ROM_LOAD( "b51-23.bin", 0x00000, 0x04000, CRC(734662de) SHA1(0058d6de68f26cd58b9eb8859e15f3ced6bd3489) )
	ROM_CONTINUE(           0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b51-16.bin", 0x000000, 0x20000, CRC(c0fcf7a5) SHA1(4550ba6d822ba12ad39576bcbed09b5fa54279e8) )
	ROM_LOAD16_BYTE( "b51-12.bin", 0x000001, 0x20000, CRC(6b36d358) SHA1(4101c110e99fe2ac1a989c84857f6438439b79a1) )
	ROM_LOAD16_BYTE( "b51-15.bin", 0x040000, 0x20000, CRC(30b2ee02) SHA1(eacd179c8760ce9ba01e234dfd3f159773e4f2ab) )
	ROM_LOAD16_BYTE( "b51-11.bin", 0x040001, 0x20000, CRC(ae9a9ac5) SHA1(f1f5216e51fea3173f5317e0dda404a29b2c45fe) )
	ROM_LOAD16_BYTE( "b51-08.bin", 0x100000, 0x20000, CRC(9f6a535c) SHA1(40d52d3f572dd87b41d89707a2ec189760d806b0) )
	ROM_LOAD16_BYTE( "b51-04.bin", 0x100001, 0x20000, CRC(03aea658) SHA1(439f08948e57c9a0f450d1319e3bc99c6fd4f82d) )
	ROM_LOAD16_BYTE( "b51-07.bin", 0x140000, 0x20000, CRC(764d4dc8) SHA1(700de70134ade3901dad51d4bf14d91f92bc5381) )
	ROM_LOAD16_BYTE( "b51-03.bin", 0x140001, 0x20000, CRC(8fd9b299) SHA1(3dc2a66678dfa13f2264bb4f5ca8a31477cc59ff) )
	ROM_LOAD16_BYTE( "b51-14.bin", 0x200000, 0x20000, CRC(dea7216e) SHA1(b97d08b24a3dd9b061ef118fd6d8b3edfa3a3008) )
	ROM_LOAD16_BYTE( "b51-10.bin", 0x200001, 0x20000, CRC(6aa97fbc) SHA1(d546dd5a276cce36e879bb7bfabbdd63d36c0f72) )
	ROM_LOAD16_BYTE( "b51-13.bin", 0x240000, 0x20000, CRC(dab28958) SHA1(da7e7fdd1d1e5a4d72b5e7df235fc304f77fa2c9) )
	ROM_LOAD16_BYTE( "b51-09.bin", 0x240001, 0x20000, CRC(cbb4f33d) SHA1(6c6560603f7fd5578a866b11031d8480bc4a9eee) )
	ROM_LOAD16_BYTE( "b51-06.bin", 0x300000, 0x20000, CRC(81bef4f0) SHA1(83b3a762b6df6f6ca193e639116345a20f874636) )
	ROM_LOAD16_BYTE( "b51-02.bin", 0x300001, 0x20000, CRC(906ba440) SHA1(9a1a147caf7eac534e739b8ad60f0c71514a64c7) )
	ROM_LOAD16_BYTE( "b51-05.bin", 0x340000, 0x20000, CRC(47976ae9) SHA1(a2b19a39d8968b886412a85c082806917e02d9fd) )
	ROM_LOAD16_BYTE( "b51-01.bin", 0x340001, 0x20000, CRC(8dab004a) SHA1(1772cdcb9d0ca5ebf429f371c041b9ae12fafcd0) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b51-18.bin", 0x00000, 0x80000, CRC(8b23ac83) SHA1(340b9e7f09c1809a332b41d3fb579f5f8cd6367f) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "b51-17.bin", 0x00000, 0x80000, CRC(d85096aa) SHA1(dac39ed182e9eda06575f1667c4c1ff9a4a56599) )
ROM_END

ROM_START( recordbr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* main cpu */
	ROM_LOAD16_BYTE( "b56-17.rom", 0x00000, 0x20000, CRC(3e0a9c35) SHA1(900a741b2abbbbe883b9d78162a88b4397af1a56) )
	ROM_LOAD16_BYTE( "b56-16.rom", 0x00001, 0x20000, CRC(b447f12c) SHA1(58ee30337836f260c7fbda728dac93f06d861ec4) )
	ROM_LOAD16_BYTE( "b56-15.rom", 0x40000, 0x20000, CRC(b346e282) SHA1(f6b4a2e9093a33d19c2eaf3ef9801179f39a83a3) )
	ROM_LOAD16_BYTE( "b56-21.rom", 0x40001, 0x20000, CRC(e5f63790) SHA1(b81db7690a989146c438609d9633ddcb1fd219dd) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )		/* sound cpu */
	ROM_LOAD( "b56-19.rom", 0x00000, 0x04000, CRC(c68085ee) SHA1(78634216a622a08c20dae0422283c4a7ed360546) )
	ROM_CONTINUE(           0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b56-04",     0x000000, 0x20000, CRC(f7afdff0) SHA1(8f8ea0e8da20913426ff3b58d7bb63bd352d3fb4) )
	ROM_LOAD16_BYTE( "b56-08",     0x000001, 0x20000, CRC(c9f0d38a) SHA1(aa22f1a06e00f90c546eebcd8b42da3e3c7d0781) )
	ROM_LOAD16_BYTE( "b56-03",     0x100000, 0x20000, CRC(4045fd44) SHA1(a84be9eedba7aed30d4f2841016784f8024d9443) )
	ROM_LOAD16_BYTE( "b56-07",     0x100001, 0x20000, CRC(0c76e4c8) SHA1(e50d1bd6e8ec967ba03bd14097a9bd560aa2decc) )
	ROM_LOAD16_BYTE( "b56-02",     0x200000, 0x20000, CRC(68c604ec) SHA1(75b26bfa53efa63b9c7a026f4226213364550cad) )
	ROM_LOAD16_BYTE( "b56-06",     0x200001, 0x20000, CRC(5fbcd302) SHA1(22e7d835643945d501edc693dbe4efc8d4d074a7) )
	ROM_LOAD16_BYTE( "b56-01.rom", 0x300000, 0x20000, CRC(766b7260) SHA1(f7d7176af614f06e8c66e890e4d194ffb6f7af73) )
	ROM_LOAD16_BYTE( "b56-05.rom", 0x300001, 0x20000, CRC(ed390378) SHA1(0275e5ead206028bfcff7ecbe11c7ab961e648ea) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b56-09.bin", 0x00000, 0x80000, CRC(7fd9ee68) SHA1(edc4455b3f6a6f30f418d03c6e53af875542a325) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "b56-10.bin", 0x00000, 0x80000, CRC(de1bce59) SHA1(aa3aea30d6f53e60d9a0d4ec767e1b261d5efc8a) )
ROM_END

ROM_START( dleague )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "c02-19a.33", 0x00000, 0x20000, CRC(7e904e45) SHA1(04ac470c973753e71fba3998099a88ab0e6fcbab) )
	ROM_LOAD16_BYTE( "c02-21a.36", 0x00001, 0x20000, CRC(18c8a32b) SHA1(507cd7a83dcb6eaefa52f2661b9f3a6fabbfbd46) )
	ROM_LOAD16_BYTE( "c02-20.34",  0x40000, 0x10000, CRC(cdf593f3) SHA1(6afbd9d8d74e6801dc991eb9fd3205057747b986) )
	ROM_LOAD16_BYTE( "c02-22.37",  0x40001, 0x10000, CRC(f50db2d7) SHA1(4f16cc42469f1e5bf6dc1aee0919712db089f9cc) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )
	ROM_LOAD( "c02-23.40", 0x00000, 0x04000, CRC(5632ee49) SHA1(90dedaf40ab526529cd7d569b78a9d5451ec3e25) )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD       ( "c02-02.15", 0x000000, 0x80000, CRC(b273f854) SHA1(5961b9fe2c49fb05f5bc3e27e05925dbef8577e9) )
	ROM_LOAD16_BYTE( "c02-06.6",  0x080000, 0x20000, CRC(b8473c7b) SHA1(8fe8d838bdba7aaaf4527ac1c5c16604922bb245) )
	ROM_LOAD16_BYTE( "c02-10.14", 0x080001, 0x20000, CRC(50c02f0f) SHA1(7d13b798c0a98387719ab738b9178ee6079327b2) )
	ROM_LOAD       ( "c02-03.17", 0x100000, 0x80000, CRC(c3fd0dcd) SHA1(43f32cefbca203bd0453e1c3d4523f0834900418) )
	ROM_LOAD16_BYTE( "c02-07.7",  0x180000, 0x20000, CRC(8c1e3296) SHA1(088b028189131186c82c61c38d5a936a0cc9830f) )
	ROM_LOAD16_BYTE( "c02-11.16", 0x180001, 0x20000, CRC(fbe548b8) SHA1(c2b453fc213c21d118810b8502836e7a2ba5626b) )
	ROM_LOAD       ( "c02-24.19", 0x200000, 0x80000, CRC(18ef740a) SHA1(27f0445c053e28267e5688627d4f91d158d4fb07) )
	ROM_LOAD16_BYTE( "c02-08.8",  0x280000, 0x20000, CRC(1a3c2f93) SHA1(0e45f8211dae8e17e67d26173262ca9831ccd283) )
	ROM_LOAD16_BYTE( "c02-12.18", 0x280001, 0x20000, CRC(b1c151c5) SHA1(3fc3d4270cad52c4a82c217b452e534d24bd8548) )
	ROM_LOAD       ( "c02-05.21", 0x300000, 0x80000, CRC(fe3a5179) SHA1(34a98969c553ee8c52aeb4fb09670a4ad572446e) )
	ROM_LOAD16_BYTE( "c02-09.9",  0x380000, 0x20000, CRC(a614d234) SHA1(dc68a6a8cf89ab82edc571853249643aa304d37f) )
	ROM_LOAD16_BYTE( "c02-13.20", 0x380001, 0x20000, CRC(8eb3194d) SHA1(98290f77a03826cdf7c8238dd35da1f9349d5cf5) )

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c02-18.22", 0x00000, 0x02000, CRC(c88f0bbe) SHA1(18c87c744fbeca35d13033e50f62e5383eb4ec2c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "c02-01.31", 0x00000, 0x80000, CRC(d5a3d1aa) SHA1(544f807015b5d854a4d8cb73e4dbae4b953fd440) )
ROM_END


static DRIVER_INIT( taitoh )
{
	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT     MONITOR  COMPANY  FULLNAME */
GAME( 1988, syvalion, 0,        syvalion, syvalion, taitoh,  ROT0,    "Taito Corporation", "Syvalion (Japan)" )
GAME( 1988, recordbr, 0,        recordbr, recordbr, taitoh,  ROT0,    "Taito Corporation Japan", "Recordbreaker (World)" )
GAME( 1990, dleague,  0,        dleague,  dleague,  taitoh,  ROT0,    "Taito Corporation", "Dynamite League (Japan)" )
