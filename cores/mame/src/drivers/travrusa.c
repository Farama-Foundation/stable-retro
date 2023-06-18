/****************************************************************************

Traverse USA / Zippy Race  (c) 1983 Irem
Shot Rider                 (c) 1984 Seibu Kaihatsu / Sigma

driver by
Lee Taylor
John Clegg
Tomasz Slanina  dox@space.pl

Notes:
- I haven't understood how char/sprite priority works. This is used for
  tunnels. I hacked it just by making the two needed colors opaque. They
  don't seem to be used anywhere else. Even if it looks like a hack, it might
  really be how the hardware works - see also notes regarding Kung Fu Master
  at the beginning of m62.c.
- shtrider has some weird colors (pink cars, green "Seibu" truck, 'no'
  on hiscore table) but there's no reason to think there's something wrong
  with the driver.

TODO:
- shtrider dip switches

****************************************************************************

Shot Rider
PCB layout:

--------------------CCCCCCCCCCC------------------

  P P              OKI                       2764
 * 2764 2764 2764      AY  AY  x 2764  2764  2764

 E           D  D                            P P

                      2764 2764 2764
--------------------------------------------------
  C  - conector
  P  - proms
  E  - connector for  big black epoxy block
  D  - dips
  *  - empty rom socket
  x  - 40 pin chip, surface scratched (6803)

Epoxy block contains main cpu (Z80)
and 2764 eprom (swapped D3/D4 and D5/D6 data lines)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/irem.h"

extern unsigned char *travrusa_videoram;

PALETTE_INIT( travrusa );
PALETTE_INIT( shtrider );
VIDEO_START( travrusa );
VIDEO_START( shtrider );
WRITE_HANDLER( travrusa_videoram_w );
WRITE_HANDLER( travrusa_scroll_x_low_w );
WRITE_HANDLER( travrusa_scroll_x_high_w );
WRITE_HANDLER( travrusa_flipscreen_w );
VIDEO_UPDATE( travrusa );
VIDEO_UPDATE( shtrider );



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },        /* Video and Color ram */
	{ 0xd000, 0xd000, input_port_0_r },	/* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },	/* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },	/* IN2 */
	{ 0xd003, 0xd003, input_port_3_r },	/* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },	/* DSW2 */
	{ 0xe000, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, travrusa_videoram_w, &travrusa_videoram },
	{ 0x9000, 0x9000, travrusa_scroll_x_low_w },
	{ 0xa000, 0xa000, travrusa_scroll_x_high_w },
	{ 0xc800, 0xc9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd000, irem_sound_cmd_w },
	{ 0xd001, 0xd001, travrusa_flipscreen_w },	/* + coin counters - not written by shtrider */
	{ 0xe000, 0xefff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( travrusa )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN3, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Fuel Reduced on Collision" )
	PORT_DIPSETTING(    0x03, "Low" )
	PORT_DIPSETTING(    0x02, "Med" )
	PORT_DIPSETTING(    0x01, "Hi" )
	PORT_DIPSETTING(    0x00, "Max" )
	PORT_DIPNAME( 0x04, 0x04, "Fuel Consumption" )
	PORT_DIPSETTING(    0x04, "Low" )
	PORT_DIPSETTING(    0x00, "Hi" )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )

	/* PORT_DIPSETTING( 0x10, "INVALID" ) */
	/* PORT_DIPSETTING( 0x00, "INVALID" ) */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Speed Type" )
	PORT_DIPSETTING(    0x08, "M/H" )
	PORT_DIPSETTING(    0x00, "Km/H" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Title" )
	PORT_DIPSETTING(    0x20, "Traverse USA" )
	PORT_DIPSETTING(    0x00, "Zippy Race" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/* same as travrusa, no "Title" switch */
INPUT_PORTS_START( motorace )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN3, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Fuel Reduced on Collision" )
	PORT_DIPSETTING(    0x03, "Low" )
	PORT_DIPSETTING(    0x02, "Med" )
	PORT_DIPSETTING(    0x01, "Hi" )
	PORT_DIPSETTING(    0x00, "Max" )
	PORT_DIPNAME( 0x04, 0x04, "Fuel Consumption" )
	PORT_DIPSETTING(    0x04, "Low" )
	PORT_DIPSETTING(    0x00, "Hi" )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )

	/* PORT_DIPSETTING( 0x10, "INVALID" ) */
	/* PORT_DIPSETTING( 0x00, "INVALID" ) */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Speed Type" )
	PORT_DIPSETTING(    0x08, "M/H" )
	PORT_DIPSETTING(    0x00, "Km/H" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( shtrider )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN2, 4 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1, 4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START      /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Speed Type" )
	PORT_DIPSETTING(    0x02, "Km/h" )
	PORT_DIPSETTING(    0x00, "Ml/h" )
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
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout sht_spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*8, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo sht_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 16 },
	{ REGION_GFX2, 0, &sht_spritelayout, 16*8, 16 },
	{ -1 }
};



static MACHINE_DRIVER_START( travrusa )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(56.75)
	MDRV_VBLANK_DURATION(1790)	/* accurate frequency, measured on a Moon Patrol board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(128+16)
	MDRV_COLORTABLE_LENGTH(16*8+16*8)

	MDRV_PALETTE_INIT(travrusa)
	MDRV_VIDEO_START(travrusa)
	MDRV_VIDEO_UPDATE(travrusa)

	/* sound hardware */
	MDRV_IMPORT_FROM(irem_audio)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( shtrider )

	MDRV_IMPORT_FROM(travrusa)

	/* video hardware */
	MDRV_GFXDECODE(sht_gfxdecodeinfo)

	MDRV_PALETTE_INIT(shtrider)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( travrusa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zippyrac.000", 0x0000, 0x2000, CRC(be066c0a) SHA1(fed0ef114b08519b99d77485b73768a838d2f06e) )
	ROM_LOAD( "zippyrac.005", 0x2000, 0x2000, CRC(145d6b34) SHA1(c9e2bd1d3e62c496e4c5057c4012b069dfcf592d) )
	ROM_LOAD( "zippyrac.006", 0x4000, 0x2000, CRC(e1b51383) SHA1(34f4476c1bcc28c53c8ffa7b614f443a329aae13) )
	ROM_LOAD( "zippyrac.007", 0x6000, 0x2000, CRC(85cd1a51) SHA1(7eb046514845cb9d2507ee24d1b2f7cc5402ac02) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, CRC(a02ad8a0) SHA1(aff80b506dbecabed2a36eb743693940f6a22d16) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zippyrac.001", 0x00000, 0x2000, CRC(aa8994dd) SHA1(9b326ce52a03d723e5c3c1b5fd4aa8fa7f70f904) )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, CRC(3a046dd1) SHA1(65c1dd1c0b5fb72ac5c04e11a577308245e4b312) )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, CRC(1cc3d3f4) SHA1(e7ee365d43d783cb6b7df37c6edeadbed35318d9) )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zippyrac.008", 0x00000, 0x2000, CRC(3e2c7a6b) SHA1(abc9eeb656ab6ed5f14e10fc988f75f21ccf037a) )
	ROM_LOAD( "zippyrac.009", 0x02000, 0x2000, CRC(13be6a14) SHA1(47861910fe4c46cd72634cf7d834be2da2a0a4f9) )
	ROM_LOAD( "zippyrac.010", 0x04000, 0x2000, CRC(6fcc9fdb) SHA1(88f878b9ebf07c5a16f8cb742016cac971ed3f10) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, CRC(c9724350) SHA1(1fac20cdc0a53d94e8f67b49d7dd71d1b9f1f7ef) ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, CRC(a1130007) SHA1(9deb0eed75dd06e86f83c819a3393158be7c9dce) ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, CRC(76062638) SHA1(7378a26cf455d9d3df90929dc665870514c34b54) ) /* sprite lookup table */
ROM_END

ROM_START( motorace )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mr.cpu",       0x0000, 0x2000, CRC(89030b0c) SHA1(dec4209385bbccff4a3c0d93d6507110ef841331) )	/* encrypted */
	ROM_LOAD( "mr1.3l",       0x2000, 0x2000, CRC(0904ed58) SHA1(2776e031cb58f99103bc35299bffd7612d954608) )
	ROM_LOAD( "mr2.3k",       0x4000, 0x2000, CRC(8a2374ec) SHA1(7159731f5ef2485e3c822e3e8e51e9583dd1c6bc) )
	ROM_LOAD( "mr3.3j",       0x6000, 0x2000, CRC(2f04c341) SHA1(ae990d9d4abdd7d6ef9d21aa62125fe2e0067623) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, CRC(a02ad8a0) SHA1(aff80b506dbecabed2a36eb743693940f6a22d16) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mr7.3e",       0x00000, 0x2000, CRC(492a60be) SHA1(9a3d6407b834eb7c3e3c8bb292ff124550a2787c) )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, CRC(3a046dd1) SHA1(65c1dd1c0b5fb72ac5c04e11a577308245e4b312) )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, CRC(1cc3d3f4) SHA1(e7ee365d43d783cb6b7df37c6edeadbed35318d9) )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mr4.3n",       0x00000, 0x2000, CRC(5cf1a0d6) SHA1(ef0883e71ee1e9c38cf3f444d9d8e79a08076b78) )
	ROM_LOAD( "mr5.3m",       0x02000, 0x2000, CRC(f75f2aad) SHA1(e4a8a3da56cbc04f0c9041afac182d1bfceb1d0d) )
	ROM_LOAD( "mr6.3k",       0x04000, 0x2000, CRC(518889a0) SHA1(70b417104ce86132cb5542813c1e0509b2260756) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, CRC(c9724350) SHA1(1fac20cdc0a53d94e8f67b49d7dd71d1b9f1f7ef) ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, CRC(a1130007) SHA1(9deb0eed75dd06e86f83c819a3393158be7c9dce) ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, CRC(76062638) SHA1(7378a26cf455d9d3df90929dc665870514c34b54) ) /* sprite lookup table */
ROM_END

ROM_START( shtrider )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.bin",   0x0000, 0x2000, CRC(eb51315c) SHA1(0101c008b6731cd8ec796fee645113e2be79bd08) ) /* was inside epoxy block with cpu, encrypted */
	ROM_LOAD( "2.bin",   0x2000, 0x2000, CRC(97675d19) SHA1(774ce4d370fcbbd8a4109df023bf21db92d2e839) )
	ROM_LOAD( "3.bin",   0x4000, 0x2000, CRC(78d051cd) SHA1(e1dc2dcfc4af35bdd5245d23977e8640d81a43f1) )
	ROM_LOAD( "4.bin",   0x6000, 0x2000, CRC(02b96eaa) SHA1(ba4d61cf57142192684c45dd22720234d3521241) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11.bin",   0xe000, 0x2000, CRC(a8396b76) SHA1(614151fb1d25930e9fee4ab290a63f8fe97adbe6) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bin",   0x0000, 0x2000, CRC(34449f79) SHA1(30aa9da07bf32282d213f63e50c564a336fd0102) )
	ROM_LOAD( "6.bin",   0x2000, 0x2000, CRC(de43653d) SHA1(a9fae236ee8e32d576123a4871ba3c46ca78ec3b) )
	ROM_LOAD( "7.bin",   0x4000, 0x2000, CRC(3445b81c) SHA1(6768d411f8c3a347b10908e757a701d5b71ca2bc) )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8.bin",   0x0000, 0x2000, CRC(4072b096) SHA1(e43482ac916a0fa259f74f99dc6ef72e86c23d9d) )
	ROM_LOAD( "9.bin",   0x2000, 0x2000, CRC(fd4cc7e6) SHA1(3852883d32354e8c90c6cf701581ebc57d830c8b) )
	ROM_LOAD( "10.bin",  0x4000, 0x2000, CRC(0a117925) SHA1(e061254428874b6153c2e9e514122373395f4da1) )

	ROM_REGION( 0x0420,  REGION_PROMS, 0 )
	ROM_LOAD( "1.bpr",   0x0000, 0x0100, CRC(e9e258e5) SHA1(b98df686e8e2afa9ed05a56e1d3acb0d7cee3888) )
	ROM_LOAD( "2.bpr",   0x0100, 0x0100, CRC(6cf4591c) SHA1(3a5795758811f4fe3518216491ac13c0d17e842f) )
	ROM_LOAD( "4.bpr",   0x0200, 0x0020, CRC(ee97c581) SHA1(a5d0ba5e03f3bcbdd72f89f0495a98cef2821e59) )
	ROM_LOAD( "3.bpr",   0x0220, 0x0100, CRC(5db47092) SHA1(8e234ee88143755a4fd5ec86a03b55be5f9c5db8) )
ROM_END



DRIVER_INIT( motorace )
{
	int A,j;
	unsigned char *rom = memory_region(REGION_CPU1);
	data8_t *buffer = malloc(0x2000);

	if (buffer)
	{
		memcpy(buffer,rom,0x2000);

		/* The first CPU ROM has the address and data lines scrambled */
		for (A = 0;A < 0x2000;A++)
		{
			j = BITSWAP16(A,15,14,13,9,7,5,3,1,12,10,8,6,4,2,0,11);
			rom[j] = BITSWAP8(buffer[A],2,7,4,1,6,3,0,5);
		}

		free(buffer);
	}
}

static DRIVER_INIT( shtrider )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);

	/* D3/D4  and  D5/D6 swapped */
	for (A = 0; A < 0x2000; A++)
		rom[A] = BITSWAP8(rom[A],7,5,6,3,4,2,1,0);
}



GAME( 1983, travrusa, 0,        travrusa, travrusa, 0,        ROT270, "Irem", "Traverse USA - Zippy Race" )
GAME( 1983, motorace, travrusa, travrusa, motorace, motorace, ROT270, "Irem (Williams license)", "MotoRace USA" )
GAMEX(1984, shtrider, 0,        shtrider, shtrider, shtrider, ROT270|ORIENTATION_FLIP_X, "Seibu Kaihatsu (Sigma license)", "Shot Rider", GAME_NO_COCKTAIL )
