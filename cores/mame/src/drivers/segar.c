/***************************************************************************

	Sega G-80 raster hardware

	Games supported:
		* Astro Blaster
		* Monster Bash
		* 005
		* Space Odyssey
		* Pig Newton
		* Sindbad Mystery

	Known bugs:
		* none at this time

****************************************************************************

	See also sega.c for the Sega G-80 Vector games.

	Many thanks go to Dave Fish for the fine detective work he did into the
	G-80 security chips (315-0064, 315-0070, 315-0076, 315-0082) which provided
	me with enough information to emulate those chips at runtime along with
	the 315-0062 Astro Blaster chip and the 315-0063 Space Odyssey chip.

	Special note (24-MAR-1999) - Sindbad Mystery does *not* use the standard
	G-80 security chip; rather, it uses the Sega System 1 encryption.

	Thanks also go to Paul Tonizzo, Clay Cowgill, John Bowes, and Kevin Klopp
	for all the helpful information, samples, and schematics!

	TODO:
	- locate Pig Newton cocktail mode?
	- verify Pig Newton and Sindbad Mystery DIPs
	- attempt Pig Newton, 005 sound
	- fix transparency issues (Pig Newton, Sindbad Mystery)
	- fix Space Odyssey background
	- figure out why Astro Blaster version 1 ends the game right away

	- Mike Balfour (mab22@po.cwru.edu)

***************************************************************************

	26/3/2000:	** Darren Hatton (UKVAC) / Adrian Purser (UKVAC) **
				Added a 3rd Astro Blaster ROM set (ASTROB2).
				Updated Dip Switches to be correct for the Astro Blaster sets.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8039/i8039.h"
#include "cpu/z80/z80.h"
#include "machine/segacrpt.h"
#include "sndhrdw/segasnd.h"
#include "segar.h"



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static INTERRUPT_GEN( segar_interrupt )
{
	if (readinputport(5) & 1)       /* get status of the F2 key */
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);	/* trigger self test */
	else
		cpu_set_irq_line(0, 0, HOLD_LINE);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

/***************************************************************************

  The Sega games store the DIP switches in a very mangled format that's
  not directly useable by MAME.  This function mangles the DIP switches
  into a format that can be used.

  Original format:
  Port 0 - 2-4, 2-8, 1-4, 1-8
  Port 1 - 2-3, 2-7, 1-3, 1-7
  Port 2 - 2-2, 2-6, 1-2, 1-6
  Port 3 - 2-1, 2-5, 1-1, 1-5
  MAME format:
  Port 6 - 1-1, 1-2, 1-3, 1-4, 1-5, 1-6, 1-7, 1-8
  Port 7 - 2-1, 2-2, 2-3, 2-4, 2-5, 2-6, 2-7, 2-8

***************************************************************************/

static READ_HANDLER( segar_ports_r )
{
	int dip1, dip2;

	dip1 = input_port_6_r(offset);
	dip2 = input_port_7_r(offset);

	switch(offset)
	{
		case 0:
		   return ((input_port_0_r(0) & 0xF0) | ((dip2 & 0x08)>>3) | ((dip2 & 0x80)>>6) |
						((dip1 & 0x08)>>1) | ((dip1 & 0x80)>>4));
		case 1:
		   return ((input_port_1_r(0) & 0xF0) | ((dip2 & 0x04)>>2) | ((dip2 & 0x40)>>5) |
						((dip1 & 0x04)>>0) | ((dip1 & 0x40)>>3));
		case 2:
		   return ((input_port_2_r(0) & 0xF0) | ((dip2 & 0x02)>>1) | ((dip2 & 0x20)>>4) |
						((dip1 & 0x02)<<1) | ((dip1 & 0x20)>>2));
		case 3:
		   return ((input_port_3_r(0) & 0xF0) | ((dip2 & 0x01)>>0) | ((dip2 & 0x10)>>3) |
						((dip1 & 0x01)<<2) | ((dip1 & 0x10)>>1));
		case 4:
		   return input_port_4_r(0);
	}

	return 0;
}



/*************************************
 *
 *	sindbad Mystery sound handling
 *
 *************************************/

static WRITE_HANDLER( sindbadm_soundport_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	/* spin for a while to let the Z80 read the command */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}


/* the data lines are flipped */
static WRITE_HANDLER( sindbadm_SN76496_0_w )
{
	int flipped = ((data >> 7) & 0x01) | ((data >> 5) & 0x02) | ((data >> 3) & 0x04) | ((data >> 1) & 0x08) |
			      ((data << 1) & 0x10) | ((data << 3) & 0x20) | ((data << 5) & 0x40) | ((data << 7) & 0x80);
	SN76496_0_w(offset, flipped);
}


static WRITE_HANDLER( sindbadm_SN76496_1_w )
{
	int flipped = ((data >> 7) & 0x01) | ((data >> 5) & 0x02) | ((data >> 3) & 0x04) | ((data >> 1) & 0x08) |
			      ((data << 1) & 0x10) | ((data << 3) & 0x20) | ((data << 5) & 0x40) | ((data << 7) & 0x80);
	SN76496_1_w(offset, flipped);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xc7ff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },	/* Misc RAM */
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe7ff, MRA_RAM },	/* Used by at least Monster Bash? */
	{ 0xe800, 0xefff, MRA_RAM },
	{ 0xf000, 0xf03f, MRA_RAM },	/* Dynamic color table */
	{ 0xf040, 0xf07f, MRA_RAM },	/* Dynamic color table for background (Monster Bash)*/
	{ 0xf080, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xffff, segar_w, &segar_mem },
	{ 0xe000, 0xe3ff, MWA_RAM, &videoram, &videoram_size },	/* handled by */
	{ 0xe800, 0xefff, MWA_RAM, &segar_characterram },    	/* the above, */
	{ 0xf000, 0xf03f, MWA_RAM, &segar_mem_colortable },     /* here only */
	{ 0xf040, 0xf07f, MWA_RAM, &segar_mem_bcolortable },    /* to initialize */
	{ 0xf800, 0xffff, MWA_RAM, &segar_characterram2 },    	/* the pointers */
MEMORY_END


static MEMORY_WRITE_START( sindbadm_writemem )
	{ 0x0000, 0xc7ff, MWA_ROM },
	{ 0xc800, 0xcfff, MWA_RAM },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, segar_characterram_w, &segar_characterram },
	{ 0xf000, 0xf03f, segar_bcolortable_w, &segar_mem_bcolortable },    /* NOTE, the two color tables are flipped! */
	{ 0xf040, 0xf07f, segar_colortable_w, &segar_mem_colortable },
	{ 0xf080, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, segar_characterram2_w, &segar_characterram2 },
MEMORY_END



/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

static PORT_READ_START( readport )
/*{0x3f, 0x3f, MRA_NOP },  // Pig Newton - read from 1D87 /*/
	{ 0x0e, 0x0e, monsterb_audio_8255_r },
	{ 0x81, 0x81, input_port_8_r },     /* only used by Sindbad Mystery */
	{ 0xf8, 0xfc, segar_ports_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0xbf, 0xbf, segar_video_port_w },
PORT_END


static PORT_WRITE_START( sindbadm_writeport )
/*      { 0x00, 0x00, ???_w },  // toggles on and off immediately (0x01, 0x00) /*/
	{ 0x41, 0x41, sindbadm_back_port_w },
	{ 0x43, 0x43, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on? */
	{ 0x80, 0x80, sindbadm_soundport_w },    /* sound commands */
PORT_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( speech_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( speech_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( monsterb_7751_readmem )
	{ 0x0000, 0x03ff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( monsterb_7751_writemem )
	{ 0x0000, 0x03ff, MWA_ROM },
MEMORY_END


static PORT_READ_START( monsterb_7751_readport )
	{ I8039_t1,  I8039_t1,  monsterb_sh_t1_r },
	{ I8039_p2,  I8039_p2,  monsterb_sh_command_r },
	{ I8039_bus, I8039_bus, monsterb_sh_rom_r },
PORT_END


static PORT_WRITE_START( monsterb_7751_writeport )
	{ I8039_p1, I8039_p1, monsterb_sh_dac_w },
	{ I8039_p2, I8039_p2, monsterb_sh_busy_w },
	{ I8039_p4, I8039_p4, monsterb_sh_offset_a0_a3_w },
	{ I8039_p5, I8039_p5, monsterb_sh_offset_a4_a7_w },
	{ I8039_p6, I8039_p6, monsterb_sh_offset_a8_a11_w },
	{ I8039_p7, I8039_p7, monsterb_sh_rom_select_w },
PORT_END


static MEMORY_READ_START( sindbadm_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END


static MEMORY_WRITE_START( sindbadm_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa003, sindbadm_SN76496_0_w },    /* the four addresses are written */
	{ 0xc000, 0xc003, sindbadm_SN76496_1_w },    /* in sequence */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

/* This fake input port is used for DIP Switch 2
   For all games except Sindbad Mystery that has different coinage */
#define COINAGE PORT_START \
	PORT_DIPNAME( 0x0f, 0x0c, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit 5/3" ) \
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 4/3" ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x0d, "1 Coin/1 Credit 5/6" ) \
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 4/5" ) \
	PORT_DIPSETTING(    0x0b, "1 Coin/1 Credit 2/3" ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x0f, "1 Coin/2 Credits 4/9" ) \
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits 5/11" ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_6C ) ) \
	PORT_DIPNAME( 0xf0, 0xc0, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit 5/3" ) \
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 4/3" ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xd0, "1 Coin/1 Credit 5/6" ) \
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 4/5" ) \
	PORT_DIPSETTING(    0xb0, "1 Coin/1 Credit 2/3" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0xf0, "1 Coin/2 Credits 4/9" ) \
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits 5/11" ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )


INPUT_PORTS_START( astrob )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Demo Speech" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( astrob2 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Demo Speech" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x80, "3" )
  /*PORT_DIPSETTING(    0x40, "3" )*/
  /*PORT_DIPSETTING(    0xc0, "3" )*/

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( astrob1 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( 005 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	/* better test those impulse */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( monsterb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL)
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x06, "40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x18, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( spaceod )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x08, "40000" )
	PORT_DIPSETTING(    0x10, "60000" )
	PORT_DIPSETTING(    0x18, "80000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( pignewt )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL)

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( pignewta )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( sindbadm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_BITX( 0x01,    0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START      /* FAKE */
	/* This fake input port is used for DIP Switch 2 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )

	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )

	PORT_START      /* IN8 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	256,
	2,
	{ 0x1000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout backlayout =
{
	8,8,
	256,
	2,
	{ 0x2000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout spaceod_layout =
{
	8,8,
	256,
	6,
	{ 0, 0x1000*8, 0x2000*8, 0x3000*8, 0x4000*8, 0x5000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xe800, &charlayout, 0x01, 0x10 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo monsterb_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xe800, &charlayout, 0x01, 0x10 },
	{ REGION_GFX1, 0x0000, &backlayout, 0x41, 0x10 },
	{ REGION_GFX1, 0x0800, &backlayout, 0x41, 0x10 },
	{ REGION_GFX1, 0x1000, &backlayout, 0x41, 0x10 },
	{ REGION_GFX1, 0x1800, &backlayout, 0x41, 0x10 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo spaceod_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xe800, &charlayout,     0x01, 0x10 },
	{ REGION_GFX1, 0x0000, &spaceod_layout, 0x41, 1 },
	{ REGION_GFX1, 0x0800, &spaceod_layout, 0x41, 1 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct Samplesinterface astrob_samples_interface =
{
	11,    /* 11 channels */
	25,    /* volume */
	astrob_sample_names
};

static struct Samplesinterface spaceod_samples_interface =
{
	12,    /* 12 channels */
	25,    /* volume */
	spaceod_sample_names
};


static struct Samplesinterface samples_interface_005 =
{
	12,    /* 12 channels */
	25,    /* volume */
	s005_sample_names
};


static struct Samplesinterface monsterb_samples_interface =
{
	2,    /* 2 channels */
	25,    /* volume */
	monsterb_sample_names
};


static struct DACinterface monsterb_dac_interface =
{
	1,
	{ 100 }
};


static struct TMS36XXinterface monsterb_tms3617_interface =
{
	1,
    { 50 },         /* mixing levels */
	{ TMS3617 },	/* TMS36xx subtype(s) */
	{ 247 },		/* base clock (one octave below A) */
	{ {0.5,0.5,0.5,0.5,0.5,0.5} }  /* decay times of voices */
};


static struct SN76496interface sn76496_interface =
{
	2,          			/* 2 chips */
	{ 4000000, 2000000 },    /* I'm assuming that the sound board is the same as System 1 */
	{ 100, 100 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( segar )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3867120)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(segar_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4+1)
	MDRV_COLORTABLE_LENGTH(16*4+1)
	
	MDRV_PALETTE_INIT(segar)
	MDRV_VIDEO_START(segar)
	MDRV_VIDEO_UPDATE(segar)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( astrob )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(segar)

	MDRV_CPU_ADD(I8035, 3120000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sega_speechboard_readmem, sega_speechboard_writemem)
	MDRV_CPU_PORTS (sega_speechboard_readport,sega_speechboard_writeport)
	MDRV_SOUND_ADD(SP0250, sega_sp0250_interface)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, astrob_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spaceod )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(segar)

	/* video hardware */
	MDRV_GFXDECODE(spaceod_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4*2+1)
	MDRV_COLORTABLE_LENGTH(16*4*2+1)

	MDRV_VIDEO_START(spaceod)
	MDRV_VIDEO_UPDATE(spaceod)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, spaceod_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 005 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(segar)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface_005)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( monsterb )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(segar)

	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(monsterb_7751_readmem,monsterb_7751_writemem)
	MDRV_CPU_PORTS(monsterb_7751_readport,monsterb_7751_writeport)

	/* video hardware */
	MDRV_GFXDECODE(monsterb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4*2+1)
	MDRV_COLORTABLE_LENGTH(16*4*2+1)

	MDRV_VIDEO_START(monsterb)
	MDRV_VIDEO_UPDATE(monsterb)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, monsterb_samples_interface)
	MDRV_SOUND_ADD(TMS36XX, monsterb_tms3617_interface)
	MDRV_SOUND_ADD(DAC,     monsterb_dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pignewt )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(segar)

	/* video hardware */
	MDRV_GFXDECODE(monsterb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4*2+1)
	MDRV_COLORTABLE_LENGTH(16*4*2+1)

	MDRV_VIDEO_START(monsterb)
	MDRV_VIDEO_UPDATE(sindbadm)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sindbadm )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ? */
	MDRV_CPU_MEMORY(readmem,sindbadm_writemem)
	MDRV_CPU_PORTS(readport,sindbadm_writeport)
	MDRV_CPU_VBLANK_INT(segar_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ? - see system1.c */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sindbadm_sound_readmem,sindbadm_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(monsterb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4*2+1)
	MDRV_COLORTABLE_LENGTH(16*4*2+1)

	MDRV_PALETTE_INIT(segar)
	MDRV_VIDEO_START(monsterb)
	MDRV_VIDEO_UPDATE(sindbadm)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( astrob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "829b",	  0x0000, 0x0800, CRC(14ae953c) SHA1(eb63d1b95faa5193db7fa6ab245e99325d519b5e) ) /* U25 */
	ROM_LOAD( "907a",     0x0800, 0x0800, CRC(a9aaaf38) SHA1(73c2b9421b267563acb33d63fbbbda818793c4c1) ) /* U1 */
	ROM_LOAD( "908a",     0x1000, 0x0800, CRC(897f2b87) SHA1(5468e68053c9a29791f367658630d5c0af332f03) ) /* U2 */
	ROM_LOAD( "909a",     0x1800, 0x0800, CRC(55a339e6) SHA1(10f603c2105761ce22c3977e3b76ab568bcde1b3) ) /* U3 */
	ROM_LOAD( "910a",     0x2000, 0x0800, CRC(7972b60a) SHA1(17f6a8a9a45c46ccb03242bac7906b235391cffc) ) /* U4 */
	ROM_LOAD( "911a",     0x2800, 0x0800, CRC(af87520f) SHA1(df92d0c38c1ab6dc0464d986d89afe3cc02f3dd6) ) /* U5 */
	ROM_LOAD( "912a",     0x3000, 0x0800, CRC(b656f929) SHA1(1b8545603f724a37727d175614fcca340714a7e3) ) /* U6 */
	ROM_LOAD( "913a",     0x3800, 0x0800, CRC(321074b3) SHA1(73c306326cea202f9481daf52a76bd9d72ef691a) ) /* U7 */
	ROM_LOAD( "914a",     0x4000, 0x0800, CRC(90d2493e) SHA1(6343cdd61a6e47edcbc6d60c8d124a56d3060e96) ) /* U8 */
	ROM_LOAD( "915a",     0x4800, 0x0800, CRC(aaf828d1) SHA1(ac175746ff24b5ebdc78a4b7ca0c37185ffa3046) ) /* U9 */
	ROM_LOAD( "916a",     0x5000, 0x0800, CRC(56d92ab9) SHA1(fd024fd178c9714fd55241f0d7081f4f3c07c463) ) /* U10 */
	ROM_LOAD( "917a",     0x5800, 0x0800, CRC(9dcdaf2d) SHA1(21d0b1e555c31ec9db14cf297be4965f9eb162ec) ) /* U11 */
	ROM_LOAD( "918a",     0x6000, 0x0800, CRC(c9d09655) SHA1(c9f642a9daab994a6d58a5f8a2f058041a0df5c2) ) /* U12 */
	ROM_LOAD( "919a",     0x6800, 0x0800, CRC(448bd318) SHA1(ba1467ebc3667e935a0153b207763ee4fcef6007) ) /* U13 */
	ROM_LOAD( "920a",     0x7000, 0x0800, CRC(3524a383) SHA1(310340218ce2881137a0781dc51fe406b220d140) ) /* U14 */
	ROM_LOAD( "921a",     0x7800, 0x0800, CRC(98c14834) SHA1(5a5a25d9eb92eaf820b10dbed1820a4210493d8f) ) /* U15 */
	ROM_LOAD( "922a",     0x8000, 0x0800, CRC(4311513c) SHA1(ec2e4d92eb128b3b75464379324f7e0341ae2494) ) /* U16 */
	ROM_LOAD( "923a",     0x8800, 0x0800, CRC(50f0462c) SHA1(659552ff8d6e6cf71f9f250d95e025371a10a2d0) ) /* U17 */
	ROM_LOAD( "924a",     0x9000, 0x0800, CRC(120a39c7) SHA1(d8fdf97290725cf9ebddab9eeb34d7adba097394) ) /* U18 */
	ROM_LOAD( "925a",     0x9800, 0x0800, CRC(790a7f4e) SHA1(16b7b8e864a8f5f59da6bf2ad17f1e4791f34abe) ) /* U19 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808b",     0x0000, 0x0800, CRC(5988c767) SHA1(3b91a8cd46aa7e714028cc40f700fea32287afb1) ) /* U7 */
	ROM_LOAD( "809a",     0x0800, 0x0800, CRC(893f228d) SHA1(41c08210d322105f5446cfaa1258c194dd078a34) ) /* U6 */
	ROM_LOAD( "810",      0x1000, 0x0800, CRC(ff0163c5) SHA1(158a12f9bf01d25c7e98f34fce56df51d49e5a85) ) /* U5 */
	ROM_LOAD( "811",      0x1800, 0x0800, CRC(219f3978) SHA1(728edb9251f7cde237fa3b005971366a099c6342) ) /* U4 */
	ROM_LOAD( "812a",     0x2000, 0x0800, CRC(410ad0d2) SHA1(9b5f05bb64a6ecfe3543025a10c6ec67de797333) ) /* U3 */
ROM_END


ROM_START( astrob2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "829b",     0x0000, 0x0800, CRC(14ae953c) SHA1(eb63d1b95faa5193db7fa6ab245e99325d519b5e) ) /* U25 */
	ROM_LOAD( "888",      0x0800, 0x0800, CRC(42601744) SHA1(6bb58384c28b2105746a2f410f5e0979609db9bf) ) /* U1 */
	ROM_LOAD( "889",      0x1000, 0x0800, CRC(dd9ab173) SHA1(35617bb4480a668bdd07b0f8a6a879fd83c53448) ) /* U2 */
	ROM_LOAD( "890",      0x1800, 0x0800, CRC(26f5b4cf) SHA1(be45e802f976b8847689ae8de7159843ac9100eb) ) /* U3 */
	ROM_LOAD( "891",      0x2000, 0x0800, CRC(6437c95f) SHA1(c9340d8edcbee254976f8ed089261fce7ae76017) ) /* U4 */
	ROM_LOAD( "892",      0x2800, 0x0800, CRC(2d3c949b) SHA1(17e3c5300793f2345ff6e28e82cd7a22f1d6e41f) ) /* U5 */
	ROM_LOAD( "893",      0x3000, 0x0800, CRC(ccdb1a76) SHA1(1c8f0555e397c5558bbfca1fa1487cc32aca8592) ) /* U6 */
	ROM_LOAD( "894",      0x3800, 0x0800, CRC(66ae5ced) SHA1(81bb6e3adcc76ffbeafefecce5fe5541a7eefc37) ) /* U7 */
	ROM_LOAD( "895",      0x4000, 0x0800, CRC(202cf3a3) SHA1(26fcccfb3e94b2a01d38c14daa66713c223efb18) ) /* U8 */
	ROM_LOAD( "896",      0x4800, 0x0800, CRC(b603fe23) SHA1(3128877355a9c5bba5cd22e9addf4c8b79ee39d2) ) /* U9 */
	ROM_LOAD( "897",      0x5000, 0x0800, CRC(989198c6) SHA1(3344bf7272e388571026c4e68a2e4e5e0ebbc5e3) ) /* U10 */
	ROM_LOAD( "898",      0x5800, 0x0800, CRC(ef2bab04) SHA1(108a9812cb9d1ec4629b0306c45ba164f94ab426) ) /* U11 */
	ROM_LOAD( "899",      0x6000, 0x0800, CRC(e0d189ee) SHA1(dcab31d64e6b2d248a82cbae9e37afe031dfc6cd) ) /* U12 */
	ROM_LOAD( "900",      0x6800, 0x0800, CRC(682d4604) SHA1(6ac0d2d8ff407cc7e10b460736ae7fbc21148640) ) /* U13 */
	ROM_LOAD( "901",      0x7000, 0x0800, CRC(9ed11c61) SHA1(dd965c06d2013acdabd958e713109eeb049d5d5e) ) /* U14 */
	ROM_LOAD( "902",      0x7800, 0x0800, CRC(b4d6c330) SHA1(922a562b5f1a8a286e6777ba7d141bd0db6e2a92) ) /* U15 */
	ROM_LOAD( "903",      0x8000, 0x0800, CRC(84acc38c) SHA1(86bed143ac2d95116e50e77b5c262d67156c6a59) ) /* U16 */
	ROM_LOAD( "904",      0x8800, 0x0800, CRC(5eba3097) SHA1(e785d1c1cea50aa25e5eea5e58a0c48fd53208c6) ) /* U17 */
	ROM_LOAD( "905",      0x9000, 0x0800, CRC(4f08f9f4) SHA1(755a825b18ed50caa7bf274a0a5c3a1b00b1c070) ) /* U18 */
	ROM_LOAD( "906",      0x9800, 0x0800, CRC(58149df1) SHA1(2bba56576a225ca47ce31a5b6dcc491546dfffec) ) /* U19 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808b",     0x0000, 0x0800, CRC(5988c767) SHA1(3b91a8cd46aa7e714028cc40f700fea32287afb1) ) /* U7 */
	ROM_LOAD( "809a",     0x0800, 0x0800, CRC(893f228d) SHA1(41c08210d322105f5446cfaa1258c194dd078a34) ) /* U6 */
	ROM_LOAD( "810",      0x1000, 0x0800, CRC(ff0163c5) SHA1(158a12f9bf01d25c7e98f34fce56df51d49e5a85) ) /* U5 */
	ROM_LOAD( "811",      0x1800, 0x0800, CRC(219f3978) SHA1(728edb9251f7cde237fa3b005971366a099c6342) ) /* U4 */
	ROM_LOAD( "812a",     0x2000, 0x0800, CRC(410ad0d2) SHA1(9b5f05bb64a6ecfe3543025a10c6ec67de797333) ) /* U3 */
ROM_END


ROM_START( astrob1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "829",      0x0000, 0x0800, CRC(5f66046e) SHA1(6aa7f94122db1a75a89c12ad9d087aec1a70d675) ) /* U25 */
	ROM_LOAD( "837",      0x0800, 0x0800, CRC(ce9c3763) SHA1(2cb4c3041905d38b040ef76f69f6197d699f9ec5) ) /* U1 */
	ROM_LOAD( "838",      0x1000, 0x0800, CRC(3557289e) SHA1(57258b149f0872c34f82fe3dc06bc1fc60d7393f) ) /* U2 */
	ROM_LOAD( "839",      0x1800, 0x0800, CRC(c88bda24) SHA1(9253306bd929e04a2136a0d9a9627dab8979baa1) ) /* U3 */
	ROM_LOAD( "840",      0x2000, 0x0800, CRC(24c9fe23) SHA1(306ad65c8c821ae90eb50416c38fe37c7acf36de) ) /* U4 */
	ROM_LOAD( "841",      0x2800, 0x0800, CRC(f153c683) SHA1(a6338a0e9867e1cbe27921182c268aeb4b09a503) ) /* U5 */
	ROM_LOAD( "842",      0x3000, 0x0800, CRC(4c5452b2) SHA1(e7b1138379ba4ad29f5ffad275164716c37c92e9) ) /* U6 */
	ROM_LOAD( "843",      0x3800, 0x0800, CRC(673161a6) SHA1(90db12ce06cd6150a924421cf9d93ad4051ccf97) ) /* U7 */
	ROM_LOAD( "844",      0x4000, 0x0800, CRC(6bfc59fd) SHA1(a5f7edfa66a25e3e84e60b5b3812d16ffdc5e409) ) /* U8 */
	ROM_LOAD( "845",      0x4800, 0x0800, CRC(018623f3) SHA1(bf264921c36ced3c332d5627b80886c3463f2237) ) /* U9 */
	ROM_LOAD( "846",      0x5000, 0x0800, CRC(4d7c5fb3) SHA1(3c0dd17cb907b8476abb4b2a16bd41dd2f107849) ) /* U10 */
	ROM_LOAD( "847",      0x5800, 0x0800, CRC(24d1d50a) SHA1(0691cccc4e58ffa07308a6a3ae80fbcef6cc8c9b) ) /* U11 */
	ROM_LOAD( "848",      0x6000, 0x0800, CRC(1c145541) SHA1(73e23161f71f4c25405e1211f41a31323845efaa) ) /* U12 */
	ROM_LOAD( "849",      0x6800, 0x0800, CRC(d378c169) SHA1(9b30ff9741429d798c8a2146d1c76a223f05495c) ) /* U13 */
	ROM_LOAD( "850",      0x7000, 0x0800, CRC(9da673ae) SHA1(817fb7cbeedefa3b5b43ca3b7914289f6908ed53) ) /* U14 */
	ROM_LOAD( "851",      0x7800, 0x0800, CRC(3d4cf9f0) SHA1(11e996f33f3a104e50d0a54a0814ea3e07735683) ) /* U15 */
	ROM_LOAD( "852",      0x8000, 0x0800, CRC(af88a97e) SHA1(fe7993101c629b296b5da05519b0990cc2b78286) ) /* U16 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808b",     0x0000, 0x0800, CRC(5988c767) SHA1(3b91a8cd46aa7e714028cc40f700fea32287afb1) ) /* U7 */
	ROM_LOAD( "809a",     0x0800, 0x0800, CRC(893f228d) SHA1(41c08210d322105f5446cfaa1258c194dd078a34) ) /* U6 */
	ROM_LOAD( "810",      0x1000, 0x0800, CRC(ff0163c5) SHA1(158a12f9bf01d25c7e98f34fce56df51d49e5a85) ) /* U5 */
	ROM_LOAD( "811",      0x1800, 0x0800, CRC(219f3978) SHA1(728edb9251f7cde237fa3b005971366a099c6342) ) /* U4 */
	ROM_LOAD( "812a",     0x2000, 0x0800, CRC(410ad0d2) SHA1(9b5f05bb64a6ecfe3543025a10c6ec67de797333) ) /* U3 */
ROM_END


ROM_START( 005 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1346b.u25",    0x0000, 0x0800, CRC(8e68533e) SHA1(a257c556d31691068ed5c991f1fb2b51da4826db) ) /* U25 */
	ROM_LOAD( "5092.u1",      0x0800, 0x0800, CRC(29e10a81) SHA1(c4b4e6c75bcf276e53f39a456d8d633c83dcf485) ) /* U1 */
	ROM_LOAD( "5093.u2",      0x1000, 0x0800, CRC(e1edc3df) SHA1(4f593546bbb0f50850dc6286cb514af6831c27a7) ) /* U2 */
	ROM_LOAD( "5094.u3",      0x1800, 0x0800, CRC(995773bb) SHA1(98dd826527853bc031edfb9a821778cc3e906150) ) /* U3 */
	ROM_LOAD( "5095.u4",      0x2000, 0x0800, CRC(f887f575) SHA1(de96573a91b60b090b1f441f1410ecad63c9467c) ) /* U4 */
	ROM_LOAD( "5096.u5",      0x2800, 0x0800, CRC(5545241e) SHA1(ee504ccaab469100137717341a1b461175ff792d) ) /* U5 */
	ROM_LOAD( "5097.u6",      0x3000, 0x0800, CRC(428edb54) SHA1(4f3df6017068d939014a8f638f28e3228acb7add) ) /* U6 */
	ROM_LOAD( "5098.u7",      0x3800, 0x0800, CRC(5bcb9d63) SHA1(c0c91bc9f75ad88a6e15c554a980d5c075725fe8) ) /* U7 */
	ROM_LOAD( "5099.u8",      0x4000, 0x0800, CRC(0ea24ba3) SHA1(95a30c9b63ef1c346df0da71af3fdecd1a75cb8f) ) /* U8 */
	ROM_LOAD( "5100.u9",      0x4800, 0x0800, CRC(a79af131) SHA1(0ba34130174e196015bc9b9c135c420209dfd524) ) /* U9 */
	ROM_LOAD( "5101.u10",     0x5000, 0x0800, CRC(8a1cdae0) SHA1(f7c617f9bdb7818e6069a981d0c8820deade134c) ) /* U10 */
	ROM_LOAD( "5102.u11",     0x5800, 0x0800, CRC(70826a15) SHA1(a86322d0e8a88534e9b78dcde42ae4c441276913) ) /* U11 */
	ROM_LOAD( "5103.u12",     0x6000, 0x0800, CRC(7f80c5b0) SHA1(00748cd5fc7f75fdca194e748524d406c006296d) ) /* U12 */
	ROM_LOAD( "5104.u13",     0x6800, 0x0800, CRC(0140930e) SHA1(f8ef894c46d3663bd89d2d817675a67075d3e0d6) ) /* U13 */
	ROM_LOAD( "5105.u14",     0x7000, 0x0800, CRC(17807a05) SHA1(bd99f5beab0155f6e4d2fab2fa5f4e147c5730d5) ) /* U14 */
	ROM_LOAD( "5106.u15",     0x7800, 0x0800, CRC(c7cdfa9d) SHA1(6ab7adc60ac7bb53a7175e8de51924008737c9ac) ) /* U15 */
	ROM_LOAD( "5107.u16",     0x8000, 0x0800, CRC(95f8a2e6) SHA1(89c92e000b3e1630380db779370cf9f5b13e5719) ) /* U16 */
	ROM_LOAD( "5108.u17",     0x8800, 0x0800, CRC(d371cacd) SHA1(8f2cdcc0b4e3b77e0958d257e37accefc5749cde) ) /* U17 */
	ROM_LOAD( "5109.u18",     0x9000, 0x0800, CRC(48a20617) SHA1(5b4bc3beda0404ff0a61bb42751b87f71817f363) ) /* U18 */
	ROM_LOAD( "5110.u19",     0x9800, 0x0800, CRC(7d26111a) SHA1(a6d3652ae606a5b75026e524c9d6aaa78300741e) ) /* U19 */
	ROM_LOAD( "5111.u20",     0xa000, 0x0800, CRC(a888e175) SHA1(4c0af94441bf51dfc852372a5b90d0830df81363) ) /* U20 */

	ROM_REGION( 0x0800, REGION_SOUND1, 0 )      /* 2k for sound */
	ROM_LOAD( "epr-1286.16",  0x0000, 0x0800, CRC(fbe0d501) SHA1(bfa277689790f835d8a43be4beee0581e1096bcc) )
ROM_END


ROM_START( monsterb )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )     /* 64k for code + space for background */
	ROM_LOAD( "1778cpu.bin",  0x0000, 0x0800, CRC(19761be3) SHA1(551a5eb958b6efac41f32e7feb2786400fcfb6d3) ) /* U25 */
	ROM_LOAD( "1779.bin",     0x0800, 0x0800, CRC(5b67dc4c) SHA1(5d2c5128b6cba2d8aa98cae8cb78dbe0c998e965) ) /* U1 */
	ROM_LOAD( "1780.bin",     0x1000, 0x0800, CRC(fac5aac6) SHA1(52a6b98760f011aa68f374801cddf0aa3efa4e69) ) /* U2 */
	ROM_LOAD( "1781.bin",     0x1800, 0x0800, CRC(3b104103) SHA1(50c68144cd76343f0e7cde35a655994f3063250f) ) /* U3 */
	ROM_LOAD( "1782.bin",     0x2000, 0x0800, CRC(c1523553) SHA1(c63d77b3add7afed54454d7b7bfc4f42276713ce) ) /* U4 */
	ROM_LOAD( "1783.bin",     0x2800, 0x0800, CRC(e0ea08c5) SHA1(1df1acd0132ee32c9cc10f55125feb95d9257706) ) /* U5 */
	ROM_LOAD( "1784.bin",     0x3000, 0x0800, CRC(48976d11) SHA1(3e64a908485d09f2949589f6f0d540627ea20c38) ) /* U6 */
	ROM_LOAD( "1785.bin",     0x3800, 0x0800, CRC(297d33ae) SHA1(af01951b41cc93bb645d4fa7f9e95bbcacd4481a) ) /* U7 */
	ROM_LOAD( "1786.bin",     0x4000, 0x0800, CRC(ef94c8f4) SHA1(a1e9b8210dc647643540643009929424d6b5a0d8) ) /* U8 */
	ROM_LOAD( "1787.bin",     0x4800, 0x0800, CRC(1b62994e) SHA1(9ab8ecac299d1e218e2bac1dd162225ca7c38c47) ) /* U9 */
	ROM_LOAD( "1788.bin",     0x5000, 0x0800, CRC(a2e32d91) SHA1(5c0ca2a8803e5b630d2f0dd9087b9022c8326f5a) ) /* U10 */
	ROM_LOAD( "1789.bin",     0x5800, 0x0800, CRC(08a172dc) SHA1(d6665904c914ebce036a320c329e1d9cb7127063) ) /* U11 */
	ROM_LOAD( "1790.bin",     0x6000, 0x0800, CRC(4e320f9d) SHA1(af39c08f1afb5396932f9dc334ad4c31c080cafe) ) /* U12 */
	ROM_LOAD( "1791.bin",     0x6800, 0x0800, CRC(3b4cba31) SHA1(6141717f6b041996971270bc387eab3092d0928c) ) /* U13 */
	ROM_LOAD( "1792.bin",     0x7000, 0x0800, CRC(7707b9f8) SHA1(0084c073fbbc453a07a32a6e51b8695a123b5235) ) /* U14 */
	ROM_LOAD( "1793.bin",     0x7800, 0x0800, CRC(a5d05155) SHA1(254012db05aeb617b590f67fa18675fa3a9dcb92) ) /* U15 */
	ROM_LOAD( "1794.bin",     0x8000, 0x0800, CRC(e4813da9) SHA1(1bfd1679ad77e2e539549811b343472890bde09a) ) /* U16 */
	ROM_LOAD( "1795.bin",     0x8800, 0x0800, CRC(4cd6ed88) SHA1(51876f5f95c2e67a8b42b19f946ccf2b3bc391e3) ) /* U17 */
	ROM_LOAD( "1796.bin",     0x9000, 0x0800, CRC(9f141a42) SHA1(278e5902ed2fbb59c24228d2c6c32407a6717757) ) /* U18 */
	ROM_LOAD( "1797.bin",     0x9800, 0x0800, CRC(ec14ad16) SHA1(7d828a6917d5c50b9c3c943271668dfd6212b366) ) /* U19 */
	ROM_LOAD( "1798.bin",     0xa000, 0x0800, CRC(86743a4f) SHA1(33d6b6a24b47bc2090636f8e89eff997eb35501d) ) /* U20 */
	ROM_LOAD( "1799.bin",     0xa800, 0x0800, CRC(41198a83) SHA1(8432fc921ab174c767c594fca1211cb20c0efd55) ) /* U21 */
	ROM_LOAD( "1800.bin",     0xb000, 0x0800, CRC(6a062a04) SHA1(cae125f5c0867898f2c0a159026da69ff5a2897f) ) /* U22 */
	ROM_LOAD( "1801.bin",     0xb800, 0x0800, CRC(f38488fe) SHA1(dd0f2c655970e8755f9ca1898313ff5fd9f11563) ) /* U23 */

	ROM_REGION( 0x1000, REGION_CPU2, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE ) /* background graphics */
	ROM_LOAD( "1516.bin",     0x0000, 0x2000, CRC(e93a2281) SHA1(61c9022edfb8fee2b7214d87d6bbed415fba9601) ) /* ??? */
	ROM_LOAD( "1517.bin",     0x2000, 0x2000, CRC(1e589101) SHA1(6805644e18e5b18b96e6a407ec217f02c8931ec2) ) /* ??? */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )      /* 8k for sound */
	ROM_LOAD( "1543snd.bin",  0x0000, 0x1000, CRC(b525ce8f) SHA1(61e541061a0a579101e52ffa2431540010b9df3e) ) /* U19 */
	ROM_LOAD( "1544snd.bin",  0x1000, 0x1000, CRC(56c79fb0) SHA1(26de83efcc97318220603f83acf4387f6d70d806) ) /* U23 */

	ROM_REGION( 0x0020, REGION_SOUND2, 0 )      /* 32 bytes for sound PROM */
	ROM_LOAD( "pr1512.u31",   0x0000, 0x0020, CRC(414ebe9b) SHA1(3df8694e3d26635d19fd4cdf02bd0998e8538b5b) )  /* U31 */

	ROM_REGION( 0x2000, REGION_USER1, 0 )		      /* background charmaps */
	ROM_LOAD( "1518a.bin",    0x0000, 0x2000, CRC(2d5932fe) SHA1(a9ca239a062e047b307cf3d0740cb6492a55abb4) ) /* ??? */
ROM_END


ROM_START( spaceod )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "so-959.dat",   0x0000, 0x0800, CRC(bbae3cd1) SHA1(2e99fd4b0db60462721b174b0db1b10b2fd13d25) ) /* U25 */
	ROM_LOAD( "so-941.dat",   0x0800, 0x0800, CRC(8b63585a) SHA1(eb064a2dca5cb44373f1acc86243a3dcca1951ee) ) /* U1 */
	ROM_LOAD( "so-942.dat",   0x1000, 0x0800, CRC(93e7d900) SHA1(dd2ec1a48a243fd1abc5467e600b8aa10dca1cfb) ) /* U2 */
	ROM_LOAD( "so-943.dat",   0x1800, 0x0800, CRC(e2f5dc10) SHA1(7b5fb00b4a46dd8eb8e0017601aae6ca771550d9) ) /* U3 */
	ROM_LOAD( "so-944.dat",   0x2000, 0x0800, CRC(b5ab01e9) SHA1(7b076a92df4cff792e2d3c2d3f78a8bcceac35aa) ) /* U4 */
	ROM_LOAD( "so-945.dat",   0x2800, 0x0800, CRC(6c5fa1b1) SHA1(35d3aed689ab8179db0aef55e9e3e48b4804837b) ) /* U5 */
	ROM_LOAD( "so-946.dat",   0x3000, 0x0800, CRC(4cef25d6) SHA1(6249ef9a906ad07bfa09fe618443538ae3f83359) ) /* U6 */
	ROM_LOAD( "so-947.dat",   0x3800, 0x0800, CRC(7220fc42) SHA1(3d939aeabb1e2eba3e788232e6df2c74b524abac) ) /* U7 */
	ROM_LOAD( "so-948.dat",   0x4000, 0x0800, CRC(94bcd726) SHA1(1f9efb81d04ea8dc970738fc388e91a70e9335ac) ) /* U8 */
	ROM_LOAD( "so-949.dat",   0x4800, 0x0800, CRC(e11e7034) SHA1(a1e80a1c0bc2cf600c84df0000007228d30f0935) ) /* U9 */
	ROM_LOAD( "so-950.dat",   0x5000, 0x0800, CRC(70a7a3b4) SHA1(4c7da7571039f583689cb388c9dd28b605d06fb2) ) /* U10 */
	ROM_LOAD( "so-951.dat",   0x5800, 0x0800, CRC(f5f0d3f9) SHA1(ea6db0e3178374190191c8c27bb1752fa6fdcc34) ) /* U11 */
	ROM_LOAD( "so-952.dat",   0x6000, 0x0800, CRC(5bf19a12) SHA1(3bc2977b15072d1fa1e28145b4c3abfb94bd8db5) ) /* U12 */
	ROM_LOAD( "so-953.dat",   0x6800, 0x0800, CRC(8066ac83) SHA1(7b2d158368cf1a325c1736a83152ee3226515a9e) ) /* U13 */
	ROM_LOAD( "so-954.dat",   0x7000, 0x0800, CRC(44ed6a0d) SHA1(974319958a6d7c023b842980e9b09f1b42f65105) ) /* U14 */
	ROM_LOAD( "so-955.dat",   0x7800, 0x0800, CRC(b5e2748d) SHA1(acac198f17f3245e2f5de2aa2b4d3f8d50311332) ) /* U15 */
	ROM_LOAD( "so-956.dat",   0x8000, 0x0800, CRC(97de45a9) SHA1(d4086dc55d25eeedb481e77da87fc13a59a65228) ) /* U16 */
	ROM_LOAD( "so-957.dat",   0x8800, 0x0800, CRC(c14b98c4) SHA1(b44fb6bee12c14190fd88bfc1b566181529dfdd7) ) /* U17 */
	ROM_LOAD( "so-958.dat",   0x9000, 0x0800, CRC(4c0a7242) SHA1(ba8317cea6986bed445ed333136e6a9649c6a89a) ) /* U18 */

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* background graphics */
	ROM_LOAD( "epr-13.dat",   0x0000, 0x1000, CRC(74bd7f9a) SHA1(daa3304916e05c35a48d7be5d283121e94a1a6c7) )
	ROM_LOAD( "epr-14.dat",   0x1000, 0x1000, CRC(d2ebd915) SHA1(c9bf888c10b8c91196696e1d42e18826eb7c98a0) )
	ROM_LOAD( "epr-15.dat",   0x2000, 0x1000, CRC(ae0e5d71) SHA1(083a24d42e8f72feb1f60b2cf004ce2be38b9910) )
	ROM_LOAD( "epr-16.dat",   0x3000, 0x1000, CRC(acdf203e) SHA1(89c4974d3f61c152976343d0ff2ab7136105e80f) )
	ROM_LOAD( "epr-17.dat",   0x4000, 0x1000, CRC(6c7490c0) SHA1(b0af14dd6510d230ea799e61f83e9c89e8ac5e19) )
	ROM_LOAD( "epr-18.dat",   0x5000, 0x1000, CRC(24a81c04) SHA1(99a2b271ecf0a00a10188f93c49318a0720aea88) )

	ROM_REGION( 0x4000, REGION_USER1, 0 )		      /* background charmaps */
	ROM_LOAD( "epr-09.dat",  0x0000, 0x1000, CRC(a87bfc0a) SHA1(7a5000f757c723a618d89c0732a5ae8818ba0f64) )
	ROM_LOAD( "epr-10.dat",  0x1000, 0x1000, CRC(8ce88100) SHA1(61177a75512bbf3629df6329e23f06fc470f36ed) )
	ROM_LOAD( "epr-11.dat",  0x2000, 0x1000, CRC(1bdbdab5) SHA1(3bb60c7bd029dd53bac7ebe640206d3980b3b426) )
	ROM_LOAD( "epr-12.dat",  0x3000, 0x1000, CRC(629a4a1f) SHA1(19badfa72207cb750364f4cab229529078b7af63) )
ROM_END


ROM_START( pignewt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cpu.u25",    0x0000, 0x0800, NO_DUMP ) /* U25 */
	ROM_LOAD( "1888c",      0x0800, 0x0800, CRC(fd18ed09) SHA1(8bba49d93ae72dbc0497a5a24991c5da26d169d3) ) /* U1 */
	ROM_LOAD( "1889c",      0x1000, 0x0800, CRC(f633f5ff) SHA1(b647bebfd8a2093b0b0b7587f7c816aade796b26) ) /* U2 */
	ROM_LOAD( "1890c",      0x1800, 0x0800, CRC(22009d7f) SHA1(2c90460ecf8d9fd9fab4a4e6e78ec634ad5f84ef) ) /* U3 */
	ROM_LOAD( "1891c",      0x2000, 0x0800, CRC(1540a7d6) SHA1(666d7f0668d585927d4fa56015f843749973da1e) ) /* U4 */
	ROM_LOAD( "1892c",      0x2800, 0x0800, CRC(960385d0) SHA1(e8a2f9fd8c9d68cd33af2457d8ba0f34a084c8bf) ) /* U5 */
	ROM_LOAD( "1893c",      0x3000, 0x0800, CRC(58c5c461) SHA1(a0748d15feaac39fa67edf4c6be5919c2a3bd0ba) ) /* U6 */
	ROM_LOAD( "1894c",      0x3800, 0x0800, CRC(5817a59d) SHA1(ca8494620c02d85fb0d5edf0291ffdef4f8a3ba6) ) /* U7 */
	ROM_LOAD( "1895c",      0x4000, 0x0800, CRC(812f67d7) SHA1(f45c0ea56a6393f3efcc6bcaab84c546a105a9b2) ) /* U8 */
	ROM_LOAD( "1896c",      0x4800, 0x0800, CRC(cc0ecdd0) SHA1(12126d1d1f3662a5b7a3bc5cdef6bb9450b703f2) ) /* U9 */
	ROM_LOAD( "1897c",      0x5000, 0x0800, CRC(7820e93b) SHA1(12a8b4bb88a74b1fe6f63e114558c0793f3f1730) ) /* U10 */
	ROM_LOAD( "1898c",      0x5800, 0x0800, CRC(e9a10ded) SHA1(67c636efe4dba8d51af489e2790852824597d7a4) ) /* U11 */
	ROM_LOAD( "1899c",      0x6000, 0x0800, CRC(d7ddf02b) SHA1(728e89e574bdec85bd8ef5a9a55c184b756e35a4) ) /* U12 */
	ROM_LOAD( "1900c",      0x6800, 0x0800, CRC(8deff4e5) SHA1(24f8a8779d8ef0a185250b324c7bbfc92cf63167) ) /* U13 */
	ROM_LOAD( "1901c",      0x7000, 0x0800, CRC(46051305) SHA1(14ebcaec8a1bef6f2e309fe6363127da1b61922a) ) /* U14 */
	ROM_LOAD( "1902c",      0x7800, 0x0800, CRC(cb937e19) SHA1(4a1755b69249bc6e12a05094d991722e42fb59f8) ) /* U15 */
	ROM_LOAD( "1903c",      0x8000, 0x0800, CRC(53239f12) SHA1(e5d2dfd683fcf350c3734f82069393d0468280a3) ) /* U16 */
	ROM_LOAD( "1913c",      0x8800, 0x0800, CRC(4652cb0c) SHA1(06bc30120cc7b25943fb70ccd8f6075b29c86a14) ) /* U17 */
	ROM_LOAD( "1914c",      0x9000, 0x0800, CRC(cb758697) SHA1(fb18b0e94d3c6495f499cef9eb1b1a9db4a216f1) ) /* U18 */
	ROM_LOAD( "1915c",      0x9800, 0x0800, CRC(9f3bad66) SHA1(985c3afce9be7afd6300d0894a6e3a3f0f0a558c) ) /* U19 */
	ROM_LOAD( "1916c",      0xa000, 0x0800, CRC(5bb6f61e) SHA1(6d2f1540a3f4dc806b2a87e16a438332cdd51b00) ) /* U20 */
	ROM_LOAD( "1917c",      0xa800, 0x0800, CRC(725e2c87) SHA1(3ea6ca8576a239236b97c3b529d2c226b3f0d28f) ) /* U21 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE ) /* background graphics */
	ROM_LOAD( "1904c.bg",   0x0000, 0x2000, CRC(e9de2c8b) SHA1(a8957585911422e07a17ec67430b30a24a6ed16c) ) /* ??? */
	ROM_LOAD( "1905c.bg",   0x2000, 0x2000, CRC(af7cfe0b) SHA1(e8a64564596d7e6e6bce00546379bd01a5b9b3d9) ) /* ??? */

	ROM_REGION( 0x4000, REGION_USER1, 0 )		      /* background charmaps */
	ROM_LOAD( "1906c.bg",  0x0000, 0x1000, CRC(c79d33ce) SHA1(8a5332a801d0db6e5f33c0d39d165819f9914e65) ) /* ??? */
	ROM_LOAD( "1907c.bg",  0x1000, 0x1000, CRC(bc839d3c) SHA1(80b1c96cac7c51e49ca40a1c5fbc156b12529d2f) ) /* ??? */
	ROM_LOAD( "1908c.bg",  0x2000, 0x1000, CRC(92cb14da) SHA1(257db7bb2758d579bcf171cda410acff1877122c) ) /* ??? */

	/* SOUND ROMS ARE PROBABLY MISSING! */
ROM_END


ROM_START( pignewta )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cpu.u25",    0x0000, 0x0800, NO_DUMP ) /* U25 */
	ROM_LOAD( "1888a",      0x0800, 0x0800, CRC(491c0835) SHA1(65c917ebcfa8e5199e9923c04626c067fda3c637) ) /* U1 */
	ROM_LOAD( "1889a",      0x1000, 0x0800, CRC(0dcf0af2) SHA1(788c24c87f44f9206c994286c7ba093d365f056f) ) /* U2 */
	ROM_LOAD( "1890a",      0x1800, 0x0800, CRC(640b8b2e) SHA1(838c3283ad92eb4390e9935e420322c4b0426800) ) /* U3 */
	ROM_LOAD( "1891a",      0x2000, 0x0800, CRC(7b8aa07f) SHA1(6d6c72ef7fd8765f1b9861928e438b854cb409cb) ) /* U4 */
	ROM_LOAD( "1892a",      0x2800, 0x0800, CRC(afc545cb) SHA1(f6f5aa654fc7eb99fe726e24c2340dee013afe17) ) /* U5 */
	ROM_LOAD( "1893a",      0x3000, 0x0800, CRC(82448619) SHA1(1206880cf8c3a9f0660a0e2af6b81d369f6a20be) ) /* U6 */
	ROM_LOAD( "1894a",      0x3800, 0x0800, CRC(4302dbfb) SHA1(acd1d2be5cb3e92ed2cbb7fb3fdd9ac6647ad21b) ) /* U7 */
	ROM_LOAD( "1895a",      0x4000, 0x0800, CRC(137ebaaf) SHA1(d1ac66e3cc575d8bb4102acf1a2f6a5bc1c1b96f) ) /* U8 */
	ROM_LOAD( "1896",       0x4800, 0x0800, CRC(1604c811) SHA1(93d14eb19bcbe446e06c178c39d73d24edf664a4) ) /* U9 */
	ROM_LOAD( "1897",       0x5000, 0x0800, CRC(3abee406) SHA1(3f48b7951364f0665e2e386e91634b10f7adbdc9) ) /* U10 */
	ROM_LOAD( "1898",       0x5800, 0x0800, CRC(a96410dc) SHA1(418801b60f3f3b57a501bb48c7aec2b71cc50e60) ) /* U11 */
	ROM_LOAD( "1899",       0x6000, 0x0800, CRC(612568a5) SHA1(ba835783b2279692ecd043288474a34646f0a23e) ) /* U12 */
	ROM_LOAD( "1900",       0x6800, 0x0800, CRC(5b231cea) SHA1(76e3bed0707a8e7348ed4eef29fe39379f916952) ) /* U13 */
	ROM_LOAD( "1901",       0x7000, 0x0800, CRC(3fd74b05) SHA1(5848f821a9e0ac5f169a810fd482f17441704423) ) /* U14 */
	ROM_LOAD( "1902",       0x7800, 0x0800, CRC(d568fc22) SHA1(60e3f041911ce644b2c475593305f505240e83b7) ) /* U15 */
	ROM_LOAD( "1903",       0x8000, 0x0800, CRC(7d16633b) SHA1(daa1af79b71c3354482e8fb89104a9fdbc88c5d2) ) /* U16 */
	ROM_LOAD( "1913",       0x8800, 0x0800, CRC(fa4be04f) SHA1(d8d5dbe74abc5b0457d16f6a173de1fe4fdafbc4) ) /* U17 */
	ROM_LOAD( "1914",       0x9000, 0x0800, CRC(08253c50) SHA1(024a9d43d03b1db5e51ebd53d30a6f08c5a8c144) ) /* U18 */
	ROM_LOAD( "1915",       0x9800, 0x0800, CRC(de786c3b) SHA1(444edc18bc6e194bbe15abd72fc73f0f8907dcc4) ) /* U19 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE ) /* background graphics */
	ROM_LOAD( "1904a.bg",   0x0000, 0x2000, NO_DUMP ) /* ??? */
	ROM_LOAD( "1905a.bg",   0x2000, 0x2000, NO_DUMP ) /* ??? */

	ROM_REGION( 0x4000, REGION_USER1, 0 )		      /* background charmaps */
	/* NOTE: No background ROMs for set A have been dumped, so the
	ROMs from set C have been copied and renamed. This is to
	provide a reminder that these ROMs still need to be dumped. */
	ROM_LOAD( "1906a.bg",  0x0000, 0x1000, BAD_DUMP CRC(c79d33ce) SHA1(8a5332a801d0db6e5f33c0d39d165819f9914e65)  ) /* ??? */
	ROM_LOAD( "1907a.bg",  0x1000, 0x1000, BAD_DUMP CRC(bc839d3c) SHA1(80b1c96cac7c51e49ca40a1c5fbc156b12529d2f)  ) /* ??? */
	ROM_LOAD( "1908a.bg",  0x2000, 0x1000, BAD_DUMP CRC(92cb14da) SHA1(257db7bb2758d579bcf171cda410acff1877122c)  ) /* ??? */

	/* SOUND ROMS ARE PROBABLY MISSING! */
ROM_END


ROM_START( sindbadm )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "epr5393.new",  0x0000, 0x2000, CRC(51f2e51e) SHA1(0fd96863d0dfaa0bab09be6fea1e7d12b9c40d68) )
	ROM_LOAD( "epr5394.new",  0x2000, 0x2000, CRC(d39ce2ee) SHA1(376065a40caa499da99e556098a03387edca5883) )
	ROM_LOAD( "epr5395.new",  0x4000, 0x2000, CRC(b1d15c82) SHA1(04f888faee0103cce8c0cd57296d75474b770632) )
	ROM_LOAD( "epr5396.new",  0x6000, 0x2000, CRC(ea9d40bf) SHA1(f61ec6e405ba3aa4166a5d6127d9e7bc940f19df) )
	ROM_LOAD( "epr5397.new",  0x8000, 0x2000, CRC(595d16dc) SHA1(ea8aa068fc45bb2ee64c4290fad4b8b51e1abe97) )
	ROM_LOAD( "epr5398.new",  0xa000, 0x2000, CRC(e57ff63c) SHA1(eac36221cb210743d3c04e51da5956623a28dbdb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound cpu (Z80) */
	ROM_LOAD( "epr5400.new",  0x0000, 0x2000, CRC(5114f18e) SHA1(343f96c728f96df5d50a9888fc87488d9440d7f4) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE ) /* background graphics */
	ROM_LOAD( "epr5428.new",  0x0000, 0x2000, CRC(f6044a1e) SHA1(19622aa0991553604236a1ff64a3e5dd1d881ed8) )
	ROM_LOAD( "epr5429.new",  0x2000, 0x2000, CRC(b23eca10) SHA1(e00ab3b50b52e16d7281ece42d73603fb188c9b3) )

	ROM_REGION( 0x8000, REGION_USER1, 0 )		      /* background charmaps */
	ROM_LOAD( "epr5424.new",  0x0000, 0x2000, CRC(4bfc2e95) SHA1(7d513df944d5768b14983f44a1e3c76930a55e9a) )
	ROM_LOAD( "epr5425.new",  0x2000, 0x2000, CRC(b654841a) SHA1(9b224fbe5f4c7bbb486a3d15550cc10e4f317631) )
	ROM_LOAD( "epr5426.new",  0x4000, 0x2000, CRC(9de0da28) SHA1(79e01005861e2426a8112544b1bc6d1c6a9ce936) )
	ROM_LOAD( "epr5427.new",  0x6000, 0x2000, CRC(a94f4d41) SHA1(fe4f412ea3680c0e5a6242827eab9e82a841d7c7) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( astrob )
{
	/* This game uses the 315-0062 security chip */
	sega_security(62);

	install_port_write_handler(0, 0x38, 0x38, sega_sh_speechboard_w);
	install_port_write_handler(0, 0x3e, 0x3f, astrob_audio_ports_w);
}


static DRIVER_INIT( 005 )
{
	/* This game uses the 315-0070 security chip */
	sega_security(70);
}


static DRIVER_INIT( monsterb )
{
	/* This game uses the 315-0082 security chip */
	sega_security(82);

	install_port_write_handler(0, 0x0c, 0x0f, monsterb_audio_8255_w);
	install_port_write_handler(0, 0xbc, 0xbc, monsterb_back_port_w);
}


static DRIVER_INIT( spaceod )
{
	/* This game uses the 315-0063 security chip */
	sega_security(63);

	install_port_write_handler(0, 0x08, 0x08, spaceod_back_port_w);
	install_port_write_handler(0, 0x09, 0x09, spaceod_backshift_clear_w);
	install_port_write_handler(0, 0x0a, 0x0a, spaceod_backshift_w);
	install_port_write_handler(0, 0x0b, 0x0c, spaceod_nobackfill_w);
	install_port_write_handler(0, 0x0d, 0x0d, spaceod_backfill_w);
	install_port_write_handler(0, 0x0e, 0x0f, spaceod_audio_ports_w);
}


static DRIVER_INIT( pignewt )
{
	/* This game uses the 315-0063? security chip */
	sega_security(63);

	install_port_write_handler(0, 0xb4, 0xb5, pignewt_back_color_w);  /* Just guessing */
	install_port_write_handler(0, 0xb8, 0xbc, pignewt_back_ports_w);   /* Just guessing */
	install_port_write_handler(0, 0xbe, 0xbe, MWA_NOP);
}


static DRIVER_INIT( sindbadm )
{
	/* This game uses an encrypted CPU */
	sindbadm_decode();
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1981, astrob,   0,       astrob,   astrob,   astrob,   ROT270, "Sega", "Astro Blaster (version 3)" )
GAME( 1981, astrob2,  astrob,  astrob,   astrob2,  astrob,   ROT270, "Sega", "Astro Blaster (version 2)" )
GAME( 1981, astrob1,  astrob,  astrob,   astrob1,  astrob,   ROT270, "Sega", "Astro Blaster (version 1)" )
GAME( 1981, 005,      0,       005,      005,      005,      ROT270, "Sega", "005" )
GAME( 1982, monsterb, 0,       monsterb, monsterb, monsterb, ROT270, "Sega", "Monster Bash" )
GAME( 1981, spaceod,  0,       spaceod,  spaceod,  spaceod,  ROT270, "Sega", "Space Odyssey" )
GAMEX(1983, pignewt,  0,       pignewt,  pignewt,  pignewt,  ROT270, "Sega", "Pig Newton (version C)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX(1983, pignewta, pignewt, pignewt,  pignewta, pignewt,  ROT270, "Sega", "Pig Newton (version A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1983, sindbadm, 0,       sindbadm, sindbadm, sindbadm, ROT270, "Sega", "Sindbad Mystery" )
