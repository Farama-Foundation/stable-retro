/***************************************************************************

Loco-Motion

driver by Nicola Salmoria

Notes:
- PROM 7a controls the video shape. This is used to hide the rightmost 4 char
  columns in Locomotion and Commando, while showing them in Jungler and
  Tactician.


CPU #1
0000-4fff ROM (empty space at 5000-5fff for diagnostic ROM)
8040-83ff video RAM (top 8 rows of screen)
8440-87ff video RAM
8840-8bff color RAM (top 8 rows of screen)
8c40-8fff color RAM
8000-803f sprite RAM #1
8800-883f sprite RAM #2
9800-9fff RAM

read:
a000      IN0
a080      IN1
a100      IN2
a180      DSW

write:
a080      watchdog reset
a100      command for the audio CPU
a180      interrupt trigger on audio CPU
a181      interrupt enable


CPU #2:
2000-23ff RAM

read:
4000      8910 #0 read
6000      8910 #1 read

write:
5000      8910 #0 control
4000      8910 #0 write
7000      8910 #1 control
6000      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/timeplt.h"


extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarx,*rallyx_radary,*rallyx_radarattr;
extern size_t rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
WRITE_HANDLER( rallyx_videoram2_w );
WRITE_HANDLER( rallyx_colorram2_w );
WRITE_HANDLER( rallyx_flipscreen_w );
PALETTE_INIT( locomotn );
VIDEO_START( rallyx );
VIDEO_UPDATE( locomotn );
VIDEO_UPDATE( jungler );
VIDEO_UPDATE( commsega );


static WRITE_HANDLER( coin_1_w )
{
	coin_counter_w(0,data & 1);
}
static WRITE_HANDLER( coin_2_w )
{
	coin_counter_w(1,data & 1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* IN2 */
	{ 0xa180, 0xa180, input_port_3_r },	/* DSW */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa004, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
/*	{ 0xa182, 0xa182, MWA_NOP },	sound mute*/
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
/*	{ 0xa187, 0xa187, MWA_NOP },	stars enable*/
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END

static MEMORY_WRITE_START( jungler_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa034, 0xa03f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
/*	{ 0xa182, 0xa182, MWA_NOP },	sound mute*/
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
/*	{ 0xa187, 0xa187, MWA_NOP },	stars enable*/
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END

static MEMORY_WRITE_START( commsega_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
/*	{ 0xa182, 0xa182, MWA_NOP },	sound mute*/
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
/*	{ 0xa187, 0xa187, MWA_NOP },	stars enable*/
	{ 0x8000, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8800, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8020, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8820, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END



INPUT_PORTS_START( jungler )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )				/* Gives 255 lives*/
INPUT_PORTS_END


INPUT_PORTS_START( locomotn )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Intermissions" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX(0,  0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END


INPUT_PORTS_START( tactcian )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10k, 80k then every 100k" )
	PORT_DIPSETTING(    0x01, "20k, 80k then every 100k" )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )			/* Mode 1*/
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	/*
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )			// Mode 2
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, "A 2C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, "A 1C/1C  B 1C/6C" )
	*/
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX(0,  0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x01, "Mode 2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


/* Due to an ingame bug at 0x1259, bit 3 of DSW1 ALSO affects the "Bonus Life" value :
     - when bit 3 is OFF, you get an extra life at 30000 points
     - when bit 3 is ON , you get an extra life at 50000 points
   In the IRQ handler (0x0038) of CPU0, there is code to give infinite lives for player 1
   when bit 3 of DSW0 is ON. I can't tell however when it is supposed to be called.
*/
INPUT_PORTS_START( commsega )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )			/* "Infinite Lives" - See notes*/
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )			/* Bonus Life : 50000 points*/
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )			/* Bonus Life : 50000 points*/
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )			/* Bonus Life : 30000 points*/
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )			/* Bonus Life : 30000 points*/
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )			/* Bonus Life : 50000 points*/
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )			/* Bonus Life : 30000 points*/
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )			/* Bonus Life : 30000 points*/
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )		/* Bonus Life : 50000 points*/
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )		/* Check code at 0x1fc5*/
	PORT_DIPSETTING(    0x40, "Easy" )					/* 16 flying enemies to kill*/
	PORT_DIPSETTING(    0x00, "Hard" )					/* 24 flying enemies to kill*/
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters (256 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8+0,8*8+1,8*8+2,8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites (64 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout dotlayout =
{
	4,4,	/* 4*4 characters */
	8,	/* 8 characters */
	2,	/* 2 bits per pixel */
	{ 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8 },
	{ 0*32, 1*32, 2*32, 3*32 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 64 },
	{ REGION_GFX1, 0, &spritelayout,    0, 64 },
	{ REGION_GFX2, 0, &dotlayout,    64*4,  1 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( tactcian )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 14318180/8)	/* 1.789772727 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(timeplt_sound_readmem,timeplt_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+4)

	MDRV_PALETTE_INIT(locomotn)
	MDRV_VIDEO_START(rallyx)
	MDRV_VIDEO_UPDATE(locomotn)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, timeplt_ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( jungler )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,jungler_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(jungler)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( locomotn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( commsega )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,commsega_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_VIDEO_UPDATE(commsega)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jungler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jungr1",       0x0000, 0x1000, CRC(5bd6ad15) SHA1(608de86e19c6726bb7d21e7dc0e936f00121a3f4) )
	ROM_LOAD( "jungr2",       0x1000, 0x1000, CRC(dc99f1e3) SHA1(942405f6c7d816139e36289126fe883a6a9a0a08) )
	ROM_LOAD( "jungr3",       0x2000, 0x1000, CRC(3dcc03da) SHA1(2c328a46511c4c9eec6515b9316a586de6503152) )
	ROM_LOAD( "jungr4",       0x3000, 0x1000, CRC(f92e9940) SHA1(d72a4d0a0ab7c9a1dcbb7925eb8530052640a234) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(f86999c3) SHA1(4660bd7826219b1bad7d9178918823196d4fd8d6) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, CRC(924262bf) SHA1(593f59630b3bd369aef0819992106b4e6e6a241f) )
	ROM_LOAD( "5m",           0x0800, 0x0800, CRC(131a08ac) SHA1(167a0710a2a153f7f7c6839d2340e5aa725ef039) )
	/* 1000-1fff empty for my convenience */

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, CRC(c59c51b7) SHA1(e8ac60fed9ba16c61a4c3c09e27f8c3f4e254014) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, CRC(55a7e6d1) SHA1(f9e4ff3b165235db2fd8dab94c43bc686c3ad29b) ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, CRC(d223f7b8) SHA1(87b62f09d4eda09c16d99d1554017d18e52b5886) ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( junglers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "5c",           0x0000, 0x1000, CRC(edd71b28) SHA1(6bdd85bc1c24ca57573252fd636e05759164de8a) )
	ROM_LOAD( "5a",           0x1000, 0x1000, CRC(61ea4d46) SHA1(575ffe9fc7d5777c8f2d2b449623c353f42a4249) )
	ROM_LOAD( "4d",           0x2000, 0x1000, CRC(557c7925) SHA1(84d8eb2fdb7ee9098805be9f457a37f51e4bc3b8) )
	ROM_LOAD( "4c",           0x3000, 0x1000, CRC(51aac9a5) SHA1(2c8a24b4ce8cec96c6e09332f3f63bd7d25ae4c6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(f86999c3) SHA1(4660bd7826219b1bad7d9178918823196d4fd8d6) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, CRC(924262bf) SHA1(593f59630b3bd369aef0819992106b4e6e6a241f) )
	ROM_LOAD( "5m",           0x0800, 0x0800, CRC(131a08ac) SHA1(167a0710a2a153f7f7c6839d2340e5aa725ef039) )
	/* 1000-1fff empty for my convenience */

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, CRC(c59c51b7) SHA1(e8ac60fed9ba16c61a4c3c09e27f8c3f4e254014) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, CRC(55a7e6d1) SHA1(f9e4ff3b165235db2fd8dab94c43bc686c3ad29b) ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, CRC(d223f7b8) SHA1(87b62f09d4eda09c16d99d1554017d18e52b5886) ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( tactcian )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tacticia.001", 0x0000, 0x1000, CRC(99163e39) SHA1(0a863f358a0bb065a9e2c41fcf4c20d370001dfe) )
	ROM_LOAD( "tacticia.002", 0x1000, 0x1000, CRC(6d3e8a69) SHA1(2b4b3f2b7401064540f59070ef6742d1f44ca839) )
	ROM_LOAD( "tacticia.003", 0x2000, 0x1000, CRC(0f71d0fa) SHA1(cb55243853b8b33034af7a6438f9a7c85a774d71) )
	ROM_LOAD( "tacticia.004", 0x3000, 0x1000, CRC(5e15f3b3) SHA1(01979f64b281a958f0a4effe2be21bf0e0a812bf) )
	ROM_LOAD( "tacticia.005", 0x4000, 0x1000, CRC(76456106) SHA1(580428f3c8cf442ee5c0f56db973644229aa8093) )
	ROM_LOAD( "tacticia.006", 0x5000, 0x1000, CRC(b33ca9ea) SHA1(0299c1cb9a3c6368bbbacb60c6f5c6854035a7bf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, CRC(97d145a7) SHA1(7aee9004287590a25e153d45b95dfaac89fbe996) )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, CRC(067f781b) SHA1(640bc7813c239e497644e53a080d81366fcd04df) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tacticia.c1",  0x0000, 0x1000, CRC(5d3ee965) SHA1(654c033291f3d139fb94f7aacbc2d1917856deb6) )
	ROM_LOAD( "tacticia.c2",  0x1000, 0x1000, CRC(e8c59c4f) SHA1(e4881f2e2e08bb8af37cc679c4e2367528ac4804) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, CRC(88b0b511) SHA1(785eded1ba761cdb59db579eb8a786516ff58152) ) /* dots */	/* tac.a7*/

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, CRC(b7ef83b7) SHA1(5ffab25c2dc5be0856a43a93711d39c4aec6660b) ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, CRC(a92796f2) SHA1(0faab2dc0f868f4023a34ecfcf972d1c86a224a0) ) /* loookup table */	/* tac.b4*/
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
/*	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) )  // video timing (not used) /*/
ROM_END

ROM_START( tactcan2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tan1",         0x0000, 0x1000, CRC(ddf38b75) SHA1(bad66fd6ae0ab3b91989fca14a8696ed855dc852) )
	ROM_LOAD( "tan2",         0x1000, 0x1000, CRC(f065ee2e) SHA1(f2362c471981af3348465f3c8a5ffb38058432a5) )
	ROM_LOAD( "tan3",         0x2000, 0x1000, CRC(2dba64fe) SHA1(8d312a6db99d2248fef2bbc590ceba333b0fde8b) )
	ROM_LOAD( "tan4",         0x3000, 0x1000, CRC(2ba07847) SHA1(3cd7cd0621ed930cb5955fc2ffe3239f6e176321) )
	ROM_LOAD( "tan5",         0x4000, 0x1000, CRC(1dae4c61) SHA1(70283b8412b0725f1c2acc281625c582a4fae39d) )
	ROM_LOAD( "tan6",         0x5000, 0x1000, CRC(2b36a18d) SHA1(bea8f36ec98975438ab267509bd9d1d1eb605945) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* sound ROMs were missing - using the ones from the other set */
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, CRC(97d145a7) SHA1(7aee9004287590a25e153d45b95dfaac89fbe996) )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, CRC(067f781b) SHA1(640bc7813c239e497644e53a080d81366fcd04df) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c1",           0x0000, 0x1000, CRC(5399471f) SHA1(66aea0df982ccbd6caaa24c258b2ba364bc1ecfd) )
	ROM_LOAD( "c2",           0x1000, 0x1000, CRC(8e8861e8) SHA1(38728418b09df06356c1e45a26cf438b93517ce5) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, CRC(88b0b511) SHA1(785eded1ba761cdb59db579eb8a786516ff58152) ) /* dots */	/* tac.a7*/

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, CRC(b7ef83b7) SHA1(5ffab25c2dc5be0856a43a93711d39c4aec6660b) ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, CRC(a92796f2) SHA1(0faab2dc0f868f4023a34ecfcf972d1c86a224a0) ) /* loookup table */	/* tac.b4*/
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, CRC(8f574815) SHA1(4f84162db9d58b64742c67dc689eb665b9862fb3) ) /* video layout (not used) */
/*	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) )  // video timing (not used) /*/
ROM_END

ROM_START( locomotn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1a.cpu",       0x0000, 0x1000, CRC(b43e689a) SHA1(7f1a0fa1ea9ff95a9d51b23ea00792ba22024282) )
	ROM_LOAD( "2a.cpu",       0x1000, 0x1000, CRC(529c823d) SHA1(714ae0af254646eb6ebc5f47422246832e89ccfb) )
	ROM_LOAD( "3.cpu",        0x2000, 0x1000, CRC(c9dbfbd1) SHA1(10ec7403053ef52d0ce4aa6eab3e82a3ea5e57ff) )
	ROM_LOAD( "4.cpu",        0x3000, 0x1000, CRC(caf6431c) SHA1(f013d8846fad9f64367b69febeb7512029a639c0) )
	ROM_LOAD( "5.cpu",        0x4000, 0x1000, CRC(64cf8dd6) SHA1(8fa1b5c4a7f136cb74833425a565fa558eeee083) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, CRC(a1105714) SHA1(6e2e264748ab90bc5e8e8167f17ff91677ef6ae7) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, CRC(5732eda9) SHA1(451de30946a9c8198c5ec83cc5c50e3ac2f9f56b) )
	ROM_LOAD( "c2.cpu",       0x1000, 0x1000, CRC(c3035300) SHA1(ddb1d658a28b973b60e2ce72fd7662537e147860) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, CRC(2ef89356) SHA1(5ed33386bab5d583358709c92f21ad9ad1a1bce9) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, CRC(75b05da0) SHA1(aee98f5389e42332f30a6882ee22ff23f37e0573) ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, CRC(aa6cf063) SHA1(08c1c9ab03eb168954b0170d40e95eed81022acd) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( gutangtn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "3d_1.bin",     0x0000, 0x1000, CRC(e9757395) SHA1(78e2f8988ed39d2ecfe1f874be370f603d5eecc1) )
	ROM_LOAD( "3e_2.bin",     0x1000, 0x1000, CRC(11d21d2e) SHA1(fd17dd481bb7bb39234fa7e9946b1cb4fa18109e) )
	ROM_LOAD( "3f_3.bin",     0x2000, 0x1000, CRC(4d80f895) SHA1(7d83f4ee34226636012a84f46af01991a28b96f6) )
	ROM_LOAD( "3h_4.bin",     0x3000, 0x1000, CRC(aa258ddf) SHA1(0f01ac0d72d8bb5a55c91a6fba3e55ed1c038b86) )
	ROM_LOAD( "3j_5.bin",     0x4000, 0x1000, CRC(52aec87e) SHA1(6516724c4e570972f070f6dab5b066ea92f56be0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, CRC(a1105714) SHA1(6e2e264748ab90bc5e8e8167f17ff91677ef6ae7) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, CRC(5732eda9) SHA1(451de30946a9c8198c5ec83cc5c50e3ac2f9f56b) )
	ROM_LOAD( "5m_c2.bin",    0x1000, 0x1000, CRC(51c542fd) SHA1(1437f8cba15811361b2c5b46085587ea3598fc88) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, CRC(2ef89356) SHA1(5ed33386bab5d583358709c92f21ad9ad1a1bce9) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, CRC(75b05da0) SHA1(aee98f5389e42332f30a6882ee22ff23f37e0573) ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, CRC(aa6cf063) SHA1(08c1c9ab03eb168954b0170d40e95eed81022acd) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( cottong )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "c1",           0x0000, 0x1000, CRC(2c256fe6) SHA1(115594c616497eec998e4e3255ec6ab6299346fa) )
	ROM_LOAD( "c2",           0x1000, 0x1000, CRC(1de5e6a0) SHA1(8bb3408a510662ff3b9b7201d2d06fe70685bf7f) )
	ROM_LOAD( "c3",           0x2000, 0x1000, CRC(01f909fe) SHA1(c80295e9f91ce25bfd28e72823b20ee6f6524a5c) )
	ROM_LOAD( "c4",           0x3000, 0x1000, CRC(a89eb3e3) SHA1(058928ade909faba06f177750f914cf1dabaefc3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "c7",           0x0000, 0x1000, CRC(3d83f6d3) SHA1(e10ed6b6ce7280697c1bc9dbe6c6e6018e1d8be4) )
	ROM_LOAD( "c8",           0x1000, 0x1000, CRC(323e1937) SHA1(75499d6c8a9032fac090a13cd4f36bd350f52dab) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c5",           0x0000, 0x1000, CRC(992d079c) SHA1(b5acd30f2e8700cc4cd852b190bd1f4163b137e8) )
	ROM_LOAD( "c6",           0x1000, 0x1000, CRC(0149ef46) SHA1(58f684a9b7b9410236b3c54ea6c0fa9853a078c5) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bpr",        0x0000, 0x0100, CRC(21fb583f) SHA1(b8c65fbdd5d8b70bf51341cd60fc2efeaab8bb82) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "2.bpr",        0x0000, 0x0020, CRC(26f42e6f) SHA1(f51578216a5d588c4d0143ce7a23d695a15a3914) ) /* palette */
	ROM_LOAD( "3.bpr",        0x0020, 0x0100, CRC(4aecc0c8) SHA1(3c1086a598d84b4bcb277556b716fd18c76c4364) ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END

ROM_START( commsega )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "csega1",       0x0000, 0x1000, CRC(92de3405) SHA1(81ef4274b13f92d6274a0a037d7dc77ba0f67a1b) )
	ROM_LOAD( "csega2",       0x1000, 0x1000, CRC(f14e2f9a) SHA1(c1a7ec1c306e07bac0bbf19b60f756650f63ae29) )
	ROM_LOAD( "csega3",       0x2000, 0x1000, CRC(941dbf48) SHA1(01d2d64fb662af423aa04507ba97997772130c54) )
	ROM_LOAD( "csega4",       0x3000, 0x1000, CRC(e0ac69b4) SHA1(3a52b2a6204b7310cfe321c582352b437de16660) )
	ROM_LOAD( "csega5",       0x4000, 0x1000, CRC(bc56ebd0) SHA1(a178cd5ba381b107e720e18f3549247477037998) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "csega8",       0x0000, 0x1000, CRC(588b4210) SHA1(43bac1bdac721567e4b5d56e9e4488165872bd6a) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "csega7",       0x0000, 0x1000, CRC(e8e374f9) SHA1(442cc6b7e7d5b9472a5c16d6f78db0c03e651e98) )
	ROM_LOAD( "csega6",       0x1000, 0x1000, CRC(cf07fd5e) SHA1(4fe351c3d093f8f5fcf95e3e921a06e44d14d2a7) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg3.bpr",      0x0000, 0x0100, CRC(ae7fd962) SHA1(118359cffb2ad3fdf09456a484aa730cb1b85a5d) ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "gg1.bpr",      0x0000, 0x0020, CRC(f69e585a) SHA1(248740b154732b6bc6f772d4bb19d654798c3739) ) /* palette */
	ROM_LOAD( "gg2.bpr",      0x0020, 0x0100, CRC(0b756e30) SHA1(8890e305547df8df108af0f89074fb1c8bed8e6c) ) /* loookup table */
	ROM_LOAD( "gg0.bpr",      0x0120, 0x0020, CRC(48c8f094) SHA1(61592209720fddc8991751edf08b6950388af42e) ) /* video layout (not used) */
	ROM_LOAD( "tt3.bpr",      0x0140, 0x0020, CRC(b8861096) SHA1(26fad384ed7a1a1e0ba719b5578e2dbb09334a25) ) /* video timing (not used) */
ROM_END



GAME( 1981, jungler,  0,        jungler,  jungler,  0, ROT90, "Konami", "Jungler" )
GAME( 1981, junglers, jungler,  jungler,  jungler,  0, ROT90, "[Konami] (Stern license)", "Jungler (Stern)" )
GAME( 1982, tactcian, 0,        tactcian, tactcian, 0, ROT90, "[Konami] (Sega license)", "Tactician (set 1)" )
GAME( 1981, tactcan2, tactcian, tactcian, tactcian, 0, ROT90, "[Konami] (Sega license)", "Tactician (set 2)" )
GAME( 1982, locomotn, 0,        locomotn, locomotn, 0, ROT90, "Konami (Centuri license)", "Loco-Motion" )
GAME( 1982, gutangtn, locomotn, locomotn, locomotn, 0, ROT90, "Konami (Sega license)", "Guttang Gottong" )
GAME( 1982, cottong,  locomotn, locomotn, locomotn, 0, ROT90, "bootleg", "Cotocoto Cottong" )
GAME( 1983, commsega, 0,        commsega, commsega, 0, ROT90, "Sega", "Commando (Sega)" )
