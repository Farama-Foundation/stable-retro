/*****************************************************************************

Raiders5 (c) 1985 Taito / UPL

	Driver by Uki

	02/Jun/2001 -

******************************************************************************

Thanks to:

David Haywood for ninjakid driver.

Howie Cohen
Frank Palazzolo
Alex Pasadyn
	for nova2001 driver.

Notes:

"Free Play" does not work properly...?

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_UPDATE( raiders5 );

extern UINT8 *raiders5_fgram;
extern size_t raiders5_fgram_size;

static UINT8 *raiders5_shared_workram;


WRITE_HANDLER( raiders5_scroll_x_w );
WRITE_HANDLER( raiders5_scroll_y_w );
WRITE_HANDLER( raiders5_flipscreen_w );

READ_HANDLER( raiders5_videoram_r );
WRITE_HANDLER( raiders5_videoram_w );
READ_HANDLER( raiders5_fgram_r );
WRITE_HANDLER( raiders5_fgram_w );

WRITE_HANDLER( raiders5_paletteram_w );

WRITE_HANDLER( raiders5_shared_workram_w )
{
	raiders5_shared_workram[offset] = data;
}

READ_HANDLER( raiders5_shared_workram_r )
{
	return raiders5_shared_workram[offset];
}

/****************************************************************************/

static MEMORY_READ_START( readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },

	{ 0x8000, 0x87ff, spriteram_r },
	{ 0x8800, 0x8fff, raiders5_fgram_r },
	{ 0x9000, 0x97ff, raiders5_videoram_r },

	{ 0xc001, 0xc001, AY8910_read_port_0_r },
	{ 0xc003, 0xc003, AY8910_read_port_1_r },

	{ 0xd000, 0xd1ff, paletteram_r },

	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem1 )
	{ 0x0000, 0x7fff, MWA_ROM },

	{ 0x8000, 0x87ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8800, 0x8fff, raiders5_fgram_w, &raiders5_fgram, &raiders5_fgram_size },
	{ 0x9000, 0x97ff, raiders5_videoram_w, &videoram, &videoram_size },

	{ 0xa000, 0xa000, raiders5_scroll_x_w },
	{ 0xa001, 0xa001, raiders5_scroll_y_w },
	{ 0xa002, 0xa002, raiders5_flipscreen_w },

	{ 0xc000, 0xc000, AY8910_control_port_0_w },
	{ 0xc001, 0xc001, AY8910_write_port_0_w },
	{ 0xc002, 0xc002, AY8910_control_port_1_w },
	{ 0xc003, 0xc003, AY8910_write_port_1_w },

	{ 0xd000, 0xd1ff, raiders5_paletteram_w, &paletteram },

	{ 0xe000, 0xe7ff, MWA_RAM, &raiders5_shared_workram },
MEMORY_END

static PORT_READ_START ( readport1 )
	{ 0x00, 0x00, IORP_NOP }, /* watchdog? */
PORT_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },

	{ 0x8001, 0x8001, AY8910_read_port_0_r },
	{ 0x8003, 0x8003, AY8910_read_port_1_r },

	{ 0x9000, 0x9000, MRA_NOP }, /* unknown */

	{ 0xa000, 0xa7ff, raiders5_shared_workram_r },

	{ 0xc000, 0xc000, MRA_NOP }, /* unknown */
	{ 0xc800, 0xc800, MRA_NOP }, /* unknown */
	{ 0xd000, 0xd000, MRA_NOP }, /* unknown */
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },

	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, AY8910_control_port_1_w },
	{ 0x8003, 0x8003, AY8910_write_port_1_w },

	{ 0xa000, 0xa7ff, raiders5_shared_workram_w },

	{ 0xe000, 0xe000, raiders5_scroll_x_w },
	{ 0xe001, 0xe001, raiders5_scroll_y_w },
	{ 0xe002, 0xe002, raiders5_flipscreen_w },
MEMORY_END

/****************************************************************************/

INPUT_PORTS_START( raiders5 )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )

	PORT_START
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x08, 0x08, "1st Bonus" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x30, 0x30, "2nd Bonus" )
	PORT_DIPSETTING(    0x30, "Every(?) 50000" )
	PORT_DIPSETTING(    0x20, "Every(?) 70000" )
	PORT_DIPSETTING(    0x10, "Every(?) 90000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Exercise" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )

	PORT_START	/* DSW2*/
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0x08, 0x08, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x08, "8 Letters" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Endless Game (If Free Play)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,   /* 8*8 characters */
	512,   /* 512 characters */
	4,     /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8192*8+0,8192*8+4,8,12,8192*8+8,8192*8+12},
	{16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7},
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,    /* 16*16 characters */
	128,	  /* 128 sprites */
	4,        /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8192*8+0,8192*8+4,8,12,8192*8+8,8192*8+12,
	16*8+0,16*8+4,16*8+8192*8+0,16*8+8192*8+4,16*8+8,16*8+12,16*8+8192*8+8,16*8+8192*8+12},
	{16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7,
	 16*16, 16*17, 16*18, 16*19, 16*20, 16*21, 16*22, 16*23},
	16*8*4
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &spritelayout, 512, 16 }, /* sprite */
	{ REGION_GFX1, 0x4000, &spritelayout, 512, 16 }, /* sprite */
	{ REGION_GFX1, 0x0000, &charlayout,     0, 16 }, /* FG */
	{ REGION_GFX1, 0x8000, &charlayout,   256, 16 }, /* BG */
	{ REGION_GFX1, 0x4000, &charlayout,   256, 16 }, /* BG (?)*/
	{ -1 } /* end of array */
};

/****************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,          /* 2 chips */
	12000000/8,    /* 1.5 MHz? */
	{ 25, 25 },
	{ input_port_0_r , input_port_2_r },
	{ input_port_1_r , input_port_3_r },
	{ 0, 0 },
	{ 0, 0 },
};

static MACHINE_DRIVER_START( raiders5 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/4)	/* 3.0MHz? */
	MDRV_CPU_MEMORY(readmem1,writemem1)
	MDRV_CPU_PORTS(readport1,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,12000000/4)	/* 3.0MHz? */
	MDRV_CPU_MEMORY(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(400)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(raiders5)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/****************************************************************************/


ROM_START( raiders5 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* CPU1 */
	ROM_LOAD( "raiders5.1", 0x0000,  0x4000, CRC(47cea11f) SHA1(0499e6627ad9c16775fdc59f2ff56dfdfc23490a) )
	ROM_LOAD( "raiders5.2", 0x4000,  0x4000, CRC(eb2ff410) SHA1(5c995b66b6301cd3cd58efd173481deaa036f842) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* CPU2 */
	ROM_LOAD( "raiders5.2", 0x0000,  0x4000, CRC(eb2ff410) SHA1(5c995b66b6301cd3cd58efd173481deaa036f842) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "raiders3.11f", 0x0000,  0x4000, CRC(30041d58) SHA1(a33087de7afb276925879898a96f418128a5a38c) )
	ROM_LOAD( "raiders4.11g", 0x4000,  0x4000, CRC(e441931c) SHA1(f39b4c25de779c671a6e2b02df64e7fed726f4da) )
	ROM_LOAD( "raiders5.11n", 0x8000,  0x4000, CRC(c0895090) SHA1(a3a1ae57ed66bc095ea9bfb26470290f67aab1fe) )
ROM_END

ROM_START( raidrs5t )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* CPU1 */
	ROM_LOAD( "raiders1.4c", 0x0000,  0x4000, CRC(4e2d5679) SHA1(a1c1603ba98814a83b92ad024ca4422aea872111) )
	ROM_LOAD( "raiders2.4d", 0x4000,  0x4000, CRC(c8604be1) SHA1(6d23f26174bb9b2f7db3a5fa6b39674fe237135b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* CPU2 */
	ROM_LOAD( "raiders2.4d", 0x0000,  0x4000, CRC(c8604be1) SHA1(6d23f26174bb9b2f7db3a5fa6b39674fe237135b) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "raiders3.11f", 0x0000,  0x4000, CRC(30041d58) SHA1(a33087de7afb276925879898a96f418128a5a38c) )
	ROM_LOAD( "raiders4.11g", 0x4000,  0x4000, CRC(e441931c) SHA1(f39b4c25de779c671a6e2b02df64e7fed726f4da) )
	ROM_LOAD( "raiders5.11n", 0x8000,  0x4000, CRC(c0895090) SHA1(a3a1ae57ed66bc095ea9bfb26470290f67aab1fe) )
ROM_END


GAME( 1985, raiders5, 0,        raiders5, raiders5, 0, ROT0, "UPL", "Raiders5" )
GAME( 1985, raidrs5t, raiders5, raiders5, raiders5, 0, ROT0, "UPL (Taito license)", "Raiders5 (Japan)" )
