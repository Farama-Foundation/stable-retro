/****************************************************************************
10 Yard Fight Driver.

L Taylor
J Clegg

Loosely based on the Kung Fu Master driver.

****************************************************************************/

#include "driver.h"
#include "sndhrdw/irem.h"
#include "vidhrdw/generic.h"


extern unsigned char *yard_scroll_x_low;
extern unsigned char *yard_scroll_x_high;
extern unsigned char *yard_scroll_y_low;
extern unsigned char *yard_score_panel_disabled;

PALETTE_INIT( yard );
VIDEO_START( yard );
WRITE_HANDLER( yard_flipscreen_w );
WRITE_HANDLER( yard_scroll_panel_w );
VIDEO_UPDATE( yard );



READ_HANDLER( mpatrol_input_port_3_r );



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },        /* Video and Color ram */
	{ 0xd000, 0xd000, input_port_0_r },	        /* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },	        /* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },	        /* IN2 */
	{ 0xd003, 0xd003, mpatrol_input_port_3_r },	/* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },	        /* DSW2 */
	{ 0xe000, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x8000, 0x8fff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x9fff, yard_scroll_panel_w },
	{ 0xc820, 0xc87f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa000, 0xa000, MWA_RAM, &yard_scroll_x_low },
	{ 0xa200, 0xa200, MWA_RAM, &yard_scroll_x_high },
	{ 0xa400, 0xa400, MWA_RAM, &yard_scroll_y_low },
	{ 0xa800, 0xa800, MWA_RAM, &yard_score_panel_disabled },
	{ 0xd000, 0xd000, irem_sound_cmd_w },
	{ 0xd001, 0xd001, yard_flipscreen_w },	/* + coin counters */
	{ 0xe000, 0xefff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( yard )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time Reduced" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "x 1.3" )
	PORT_DIPSETTING(    0x04, "x 1.5" )
	PORT_DIPSETTING(    0x00, "x 1.8" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Gets filled in based on the coin mode */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Slow Motion", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	/* Fake port to support the two different coin modes */
	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage Mode 1" ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */
 	PORT_DIPNAME( 0x30, 0x30, "Coin A  Mode 2" )   /* mapped on coin mode 2 */
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B  Mode 2" )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

/* exactly the same as yard, only difference is the Allow Continue dip switch */
/* Also, the Cabinet dip switch doesn't seem to work. */
INPUT_PORTS_START( vsyard )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Continue (Vs. Mode Only)" )
	PORT_DIPSETTING( 0x01, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "SW2A" )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time Reduced" )
	PORT_DIPSETTING( 0x0c, "Normal" )
	PORT_DIPSETTING( 0x08, "x 1.3" )
	PORT_DIPSETTING( 0x04, "x 1.5" )
	PORT_DIPSETTING( 0x00, "x 1.8" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Gets filled in based on the coin mode */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Slow Motion", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "SW6B" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	/* Fake port to support the two different coin modes */
	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage Mode 1" ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */
 	PORT_DIPNAME( 0x30, 0x30, "Coin A  Mode 2" )   /* mapped on coin mode 2 */
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B  Mode 2" )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*0x4000*8, 0x4000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 32 },	/* use colors 0-255 */
	{ REGION_GFX2, 0, &spritelayout,  32*8, 32 },	/* use colors 256-271 with lookup table */
	/* bitmapped radar uses colors 272-527 */
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( yard )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(1790)	/* accurate frequency, measured on a Moon Patrol board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256+16+256)
	MDRV_COLORTABLE_LENGTH(32*8+32*8)

	MDRV_PALETTE_INIT(yard)
	MDRV_VIDEO_START(yard)
	MDRV_VIDEO_UPDATE(yard)

	/* sound hardware */
	MDRV_IMPORT_FROM(irem_audio)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( yard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "yf-a-3p",      0x0000, 0x2000, CRC(4586114f) SHA1(a31c68770e7a7eed805c5ba46af302c2895e3cee) )
	ROM_LOAD( "yf-a-3n",      0x2000, 0x2000, CRC(947fa760) SHA1(bd6c2ee6e6800b063b81dbdd9fc929120019439d) )
	ROM_LOAD( "yf-a-3m",      0x4000, 0x2000, CRC(d4975633) SHA1(84a506ae680a9dd26ef6f33880400e965ccf8260) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "yf-s-3b",      0x8000, 0x2000, CRC(0392a60c) SHA1(68030504eafc58db250099edd3c3323bdb9eff6b) )
	ROM_LOAD( "yf-s-1b",      0xa000, 0x2000, CRC(6588f41a) SHA1(209305efc68171886427216b9a0b37333f40daa8) )
	ROM_LOAD( "yf-s-3a",      0xc000, 0x2000, CRC(bd054e44) SHA1(f10c32c70d60680229fc0891d0e1308015fa69d6) )
	ROM_LOAD( "yf-s-1a",      0xe000, 0x2000, CRC(2490d4c3) SHA1(e4da7b01e8ad075b7e3c8beb6668faff72db9aa2) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "yf-a-3e",      0x00000, 0x2000, CRC(77e9e9cc) SHA1(90b0226fc125713dbee2804aeceeb5aa2c8e275e) )	/* chars */
	ROM_LOAD( "yf-a-3d",      0x02000, 0x2000, CRC(854d5ff4) SHA1(9ba09bfabf159facb57faecfe73a6258fa48d152) )
	ROM_LOAD( "yf-a-3c",      0x04000, 0x2000, CRC(0cd8ffad) SHA1(bd1262de3823c34f7394b718477fb5bc58a6e293) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "yf-b-5b",      0x00000, 0x2000, CRC(1299ae30) SHA1(07d47f827d8bc78a41011ec02ab64036fb8a7a18) )	/* sprites */
	ROM_LOAD( "yf-b-5c",      0x02000, 0x2000, CRC(8708b888) SHA1(8c4f305a339f23ec8ed40dfd72fac0f62ee65378) )
	ROM_LOAD( "yf-b-5f",      0x04000, 0x2000, CRC(d9bb8ab8) SHA1(1325308b4c85355298fec4aa3e5fec1b4b13ad86) )
	ROM_LOAD( "yf-b-5e",      0x06000, 0x2000, CRC(47077e8d) SHA1(5f78b15fb360e9926ef11841d5d86f2bd9af04d1) )
	ROM_LOAD( "yf-b-5j",      0x08000, 0x2000, CRC(713ef31f) SHA1(b48df9ed4f26fded3c7eaac3a52b580b2dd60477) )
	ROM_LOAD( "yf-b-5k",      0x0a000, 0x2000, CRC(f49651cc) SHA1(5b87d7360bcd5883ec265b2a01a3e02e10a85345) )

	ROM_REGION( 0x0520, REGION_PROMS, 0 )
	ROM_LOAD( "yard.1c",      0x0000, 0x0100, CRC(08fa5103) SHA1(98af48dafbbaa42f58232bf74ccbf5da41723e71) ) /* chars palette low 4 bits */
	ROM_LOAD( "yard.1d",      0x0100, 0x0100, CRC(7c04994c) SHA1(790bf1616335b9df4943cffcafa48d8e8aee009e) ) /* chars palette high 4 bits */
	ROM_LOAD( "yard.1f",      0x0200, 0x0020, CRC(b8554da5) SHA1(963ca815b5f791b8a7b0937a5d392d5203049eb3) ) /* sprites palette */
	ROM_LOAD( "yard.2h",      0x0220, 0x0100, CRC(e1cdfb06) SHA1(a8cc3456cfc272e3faac80370b2298d8e1f8c2fe) ) /* sprites lookup table */
	ROM_LOAD( "yard.2n",      0x0320, 0x0100, CRC(cd85b646) SHA1(5268db705006058eec308afe474f4df3c15465bb) ) /* radar palette low 4 bits */
	ROM_LOAD( "yard.2m",      0x0420, 0x0100, CRC(45384397) SHA1(e4c662ee81aef63efd8b4a45f85c4a78dc2d419e) ) /* radar palette high 4 bits */
ROM_END

ROM_START( vsyard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a-3p",         0x0000, 0x2000, CRC(1edac08f) SHA1(c6a3290e9dba663dccf0613853abfab8e912477d) )
	ROM_LOAD( "vyf-a-3m",     0x2000, 0x2000, CRC(3b9330f8) SHA1(b35fe72cf724cfb887906060bbcf40b0c896ccf0) )
	ROM_LOAD( "a-3m",         0x4000, 0x2000, CRC(cf783dad) SHA1(0b1b875ac65ba90c92ca06d0aa01c477b7427322) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "yf-s-3b",      0x8000, 0x2000, CRC(0392a60c) SHA1(68030504eafc58db250099edd3c3323bdb9eff6b) )
	ROM_LOAD( "yf-s-1b",      0xa000, 0x2000, CRC(6588f41a) SHA1(209305efc68171886427216b9a0b37333f40daa8) )
	ROM_LOAD( "yf-s-3a",      0xc000, 0x2000, CRC(bd054e44) SHA1(f10c32c70d60680229fc0891d0e1308015fa69d6) )
	ROM_LOAD( "yf-s-1a",      0xe000, 0x2000, CRC(2490d4c3) SHA1(e4da7b01e8ad075b7e3c8beb6668faff72db9aa2) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vyf-a-3a",     0x00000, 0x2000, CRC(354d7330) SHA1(0dac87e502d5e9089c4e5ca87c7626940a17e9b2) )	/* chars */
	ROM_LOAD( "vyf-a-3c",     0x02000, 0x2000, CRC(f48eedca) SHA1(6aef3208de8b1dd4078de20c0b5ce96219c79d40) )
	ROM_LOAD( "vyf-a-3d",     0x04000, 0x2000, CRC(7d1b4d93) SHA1(9389de1230b93f529c492af6fb911c00280cae8a) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "yf-b-5b",      0x00000, 0x2000, CRC(1299ae30) SHA1(07d47f827d8bc78a41011ec02ab64036fb8a7a18) )	/* sprites */
	ROM_LOAD( "yf-b-5c",      0x02000, 0x2000, CRC(8708b888) SHA1(8c4f305a339f23ec8ed40dfd72fac0f62ee65378) )
	ROM_LOAD( "yf-b-5f",      0x04000, 0x2000, CRC(d9bb8ab8) SHA1(1325308b4c85355298fec4aa3e5fec1b4b13ad86) )
	ROM_LOAD( "yf-b-5e",      0x06000, 0x2000, CRC(47077e8d) SHA1(5f78b15fb360e9926ef11841d5d86f2bd9af04d1) )
	ROM_LOAD( "yf-b-5j",      0x08000, 0x2000, CRC(713ef31f) SHA1(b48df9ed4f26fded3c7eaac3a52b580b2dd60477) )
	ROM_LOAD( "yf-b-5k",      0x0a000, 0x2000, CRC(f49651cc) SHA1(5b87d7360bcd5883ec265b2a01a3e02e10a85345) )

	ROM_REGION( 0x0520, REGION_PROMS, 0 )
	ROM_LOAD( "yard.1c",      0x0000, 0x0100, CRC(08fa5103) SHA1(98af48dafbbaa42f58232bf74ccbf5da41723e71) ) /* chars palette low 4 bits */
	ROM_LOAD( "yard.1d",      0x0100, 0x0100, CRC(7c04994c) SHA1(790bf1616335b9df4943cffcafa48d8e8aee009e) ) /* chars palette high 4 bits */
	ROM_LOAD( "yard.1f",      0x0200, 0x0020, CRC(b8554da5) SHA1(963ca815b5f791b8a7b0937a5d392d5203049eb3) ) /* sprites palette */
	ROM_LOAD( "yard.2h",      0x0220, 0x0100, CRC(e1cdfb06) SHA1(a8cc3456cfc272e3faac80370b2298d8e1f8c2fe) ) /* sprites lookup table */
	ROM_LOAD( "yard.2n",      0x0320, 0x0100, CRC(cd85b646) SHA1(5268db705006058eec308afe474f4df3c15465bb) ) /* radar palette low 4 bits */
	ROM_LOAD( "yard.2m",      0x0420, 0x0100, CRC(45384397) SHA1(e4c662ee81aef63efd8b4a45f85c4a78dc2d419e) ) /* radar palette high 4 bits */
ROM_END

ROM_START( vsyard2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vyf-a-3n",     0x0000, 0x2000, CRC(418e01fc) SHA1(56a6515735cd88ec803e24574a28aef823a5d36b) )
	ROM_LOAD( "vyf-a-3m",     0x2000, 0x2000, CRC(3b9330f8) SHA1(b35fe72cf724cfb887906060bbcf40b0c896ccf0) )
	ROM_LOAD( "vyf-a-3k",     0x4000, 0x2000, CRC(a0ec15bb) SHA1(a5ce9341e9d05e33c025ac62a27faf738c88326e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "yf-s-3b",      0x8000, 0x2000, CRC(0392a60c) SHA1(68030504eafc58db250099edd3c3323bdb9eff6b) )
	ROM_LOAD( "yf-s-1b",      0xa000, 0x2000, CRC(6588f41a) SHA1(209305efc68171886427216b9a0b37333f40daa8) )
	ROM_LOAD( "yf-s-3a",      0xc000, 0x2000, CRC(bd054e44) SHA1(f10c32c70d60680229fc0891d0e1308015fa69d6) )
	ROM_LOAD( "yf-s-1a",      0xe000, 0x2000, CRC(2490d4c3) SHA1(e4da7b01e8ad075b7e3c8beb6668faff72db9aa2) )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vyf-a-3a",     0x00000, 0x2000, CRC(354d7330) SHA1(0dac87e502d5e9089c4e5ca87c7626940a17e9b2) )	/* chars */
	ROM_LOAD( "vyf-a-3c",     0x02000, 0x2000, CRC(f48eedca) SHA1(6aef3208de8b1dd4078de20c0b5ce96219c79d40) )
	ROM_LOAD( "vyf-a-3d",     0x04000, 0x2000, CRC(7d1b4d93) SHA1(9389de1230b93f529c492af6fb911c00280cae8a) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "yf-b-5b",      0x00000, 0x2000, CRC(1299ae30) SHA1(07d47f827d8bc78a41011ec02ab64036fb8a7a18) )	/* sprites */
	ROM_LOAD( "yf-b-5c",      0x02000, 0x2000, CRC(8708b888) SHA1(8c4f305a339f23ec8ed40dfd72fac0f62ee65378) )
	ROM_LOAD( "yf-b-5f",      0x04000, 0x2000, CRC(d9bb8ab8) SHA1(1325308b4c85355298fec4aa3e5fec1b4b13ad86) )
	ROM_LOAD( "yf-b-5e",      0x06000, 0x2000, CRC(47077e8d) SHA1(5f78b15fb360e9926ef11841d5d86f2bd9af04d1) )
	ROM_LOAD( "yf-b-5j",      0x08000, 0x2000, CRC(713ef31f) SHA1(b48df9ed4f26fded3c7eaac3a52b580b2dd60477) )
	ROM_LOAD( "yf-b-5k",      0x0a000, 0x2000, CRC(f49651cc) SHA1(5b87d7360bcd5883ec265b2a01a3e02e10a85345) )

	ROM_REGION( 0x0520, REGION_PROMS, 0 )
	ROM_LOAD( "yard.1c",      0x0000, 0x0100, CRC(08fa5103) SHA1(98af48dafbbaa42f58232bf74ccbf5da41723e71) ) /* chars palette low 4 bits */
	ROM_LOAD( "yard.1d",      0x0100, 0x0100, CRC(7c04994c) SHA1(790bf1616335b9df4943cffcafa48d8e8aee009e) ) /* chars palette high 4 bits */
	ROM_LOAD( "yard.1f",      0x0200, 0x0020, CRC(b8554da5) SHA1(963ca815b5f791b8a7b0937a5d392d5203049eb3) ) /* sprites palette */
	ROM_LOAD( "yard.2h",      0x0220, 0x0100, CRC(e1cdfb06) SHA1(a8cc3456cfc272e3faac80370b2298d8e1f8c2fe) ) /* sprites lookup table */
	ROM_LOAD( "yard.2n",      0x0320, 0x0100, CRC(cd85b646) SHA1(5268db705006058eec308afe474f4df3c15465bb) ) /* radar palette low 4 bits */
	ROM_LOAD( "yard.2m",      0x0420, 0x0100, CRC(45384397) SHA1(e4c662ee81aef63efd8b4a45f85c4a78dc2d419e) ) /* radar palette high 4 bits */
ROM_END



GAME( 1983, yard,    0,    yard, yard,   0, ROT0, "Irem", "10 Yard Fight (Japan)" )
GAME( 1984, vsyard,  yard, yard, vsyard, 0, ROT0, "Irem", "10 Yard Fight (Vs. version World, 11-05-84)" )
GAME( 1984, vsyard2, yard, yard, vsyard, 0, ROT0, "Irem", "10 Yard Fight (Vs. version Japan, set 2)" )
