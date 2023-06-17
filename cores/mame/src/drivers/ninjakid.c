/*******************************************************************************
 Ninja Kid / Ninjakun Majou no Bouken | (c) 1984 UPL / Taito
********************************************************************************
 Driver by David Haywood
 with help from Steph and Phil Stroffolino

 Last Changes: 5 Mar 2001

 This driver was started after interest was shown in the game by a poster at
 various messageboards going under the name of 'ninjakid'  I decided to attempt
 a driver for this game to gain some experience with Z80 & Multi-processor
 games.

Hold P1 Start after a reset to skip the startup memory tests.

Change Log:
5 Mar - Added Saved State Support (DJH)
8 Jun - Added palette animation, Fixed FG priority. (Uki)
9 Jun - Fixed BG scroll handling, Fixed CPU clock.
*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "state.h"

extern WRITE_HANDLER( ninjakid_bg_videoram_w );
extern WRITE_HANDLER( ninjakid_fg_videoram_w );
extern READ_HANDLER( ninjakid_bg_videoram_r );

extern READ_HANDLER( ninjakun_io_8000_r );
extern WRITE_HANDLER( ninjakun_io_8000_w );

extern VIDEO_START( ninjakid );
extern VIDEO_UPDATE( ninjakid );
extern WRITE_HANDLER( ninjakun_flipscreen_w );

extern WRITE_HANDLER( ninjakun_paletteram_w );

/******************************************************************************/

static UINT8 *ninjakid_gfx_rom;

static READ_HANDLER( ninjakid_shared_rom_r ){
	return ninjakid_gfx_rom[offset];
}

/* working RAM is shared, but an address line is inverted */
static UINT8 *shareram;

static WRITE_HANDLER( shareram_w ){
	shareram[offset^0x400] = data;
}
static READ_HANDLER( shareram_r ){
	return shareram[offset^0x400];
}

/*******************************************************************************
 0xA000 Read / Write Handlers
*******************************************************************************/

static UINT8 ninjakun_io_a002_ctrl;

static READ_HANDLER( ninjakun_io_A002_r ){
	return ninjakun_io_a002_ctrl | readinputport(2); /* vblank */
}

static WRITE_HANDLER( cpu1_A002_w ){
	if( data == 0x80 ) ninjakun_io_a002_ctrl |= 0x04;
	if( data == 0x40 ) ninjakun_io_a002_ctrl &= ~0x08;
}

static WRITE_HANDLER( cpu2_A002_w ){
	if( data == 0x40 ) ninjakun_io_a002_ctrl |= 0x08;
	if( data == 0x80 ) ninjakun_io_a002_ctrl &= ~0x04;
}

/*******************************************************************************
 Memory Maps
*******************************************************************************/

static MEMORY_READ_START( ninjakid_primary_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8003, ninjakun_io_8000_r },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },
	{ 0xa002, 0xa002, ninjakun_io_A002_r },
	{ 0xc000, 0xc7ff, MRA_RAM },	/* tilemaps */
	{ 0xc800, 0xcfff, ninjakid_bg_videoram_r },
    { 0xd000, 0xd7ff, MRA_RAM },	/* spriteram */
    { 0xd800, 0xd9ff, paletteram_r },
    { 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( ninjakid_primary_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x7fff, MWA_ROM, &ninjakid_gfx_rom },
	{ 0x8000, 0x8003, ninjakun_io_8000_w },
	{ 0xa002, 0xa002, cpu1_A002_w },
	{ 0xa003, 0xa003, ninjakun_flipscreen_w },
	{ 0xc000, 0xc7ff, ninjakid_fg_videoram_w, &videoram },
	{ 0xc800, 0xcfff, ninjakid_bg_videoram_w },
	{ 0xd000, 0xd7ff, MWA_RAM, &spriteram },
	{ 0xd800, 0xd9ff, ninjakun_paletteram_w, &paletteram },
	{ 0xe000, 0xe7ff, MWA_RAM, &shareram },
MEMORY_END

static MEMORY_READ_START( ninjakid_secondary_readmem )
    { 0x0000, 0x1fff, MRA_ROM },
    { 0x2000, 0x7fff, ninjakid_shared_rom_r },
	{ 0x8000, 0x8003, ninjakun_io_8000_r },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },
	{ 0xa002, 0xa002, ninjakun_io_A002_r },
	{ 0xc000, 0xc7ff, videoram_r },		/* tilemaps */
	{ 0xc800, 0xcfff, ninjakid_bg_videoram_r },
    { 0xd000, 0xd7ff, spriteram_r },	/* shareram */
    { 0xd800, 0xd9ff, paletteram_r },
    { 0xe000, 0xe7ff, shareram_r },
MEMORY_END

static MEMORY_WRITE_START( ninjakid_secondary_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x8003, ninjakun_io_8000_w },
	{ 0xa002, 0xa002, cpu2_A002_w },
	{ 0xa003, 0xa003, ninjakun_flipscreen_w },
	{ 0xc000, 0xc7ff, ninjakid_fg_videoram_w },
	{ 0xc800, 0xcfff, ninjakid_bg_videoram_w },
	{ 0xd000, 0xd7ff, spriteram_w },	/* shareram */
	{ 0xd800, 0xd9ff, ninjakun_paletteram_w },
    { 0xe000, 0xe7ff, shareram_w },
MEMORY_END

/*******************************************************************************
 GFX Decoding Information
*******************************************************************************/

static struct GfxLayout tile_layout =
{
	8,8,	/* tile size */
	0x400,	/* number of tiles */
	4,		/* bits per pixel */
	{ 0, 1, 2, 3 }, /* plane offsets */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 }, /* x offsets */
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 }, /* y offsets */
	256
};

static struct GfxLayout sprite_layout =
{
	16,16,	/* tile size */
	0x100,	/* number of tiles */
	4,		/* bits per pixel */
	{ 0, 1, 2, 3 }, /* plane offsets */
	{
		0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		256+0*4, 256+1*4, 256+2*4, 256+3*4, 256+4*4, 256+5*4, 256+6*4, 256+7*4,
	}, /* x offsets */
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		512+0*32, 512+1*32, 512+2*32, 512+3*32, 512+4*32, 512+5*32, 512+6*32, 512+7*32
	}, /* y offsets */
	1024
};

static struct GfxDecodeInfo ninjakid_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,		0x000, 0x10 },
	{ REGION_GFX2, 0, &tile_layout,		0x100, 0x10 },
	{ REGION_GFX1, 0, &sprite_layout,	0x200, 0x10 },
	{ -1 }
};

/*******************************************************************************
 Machine Driver Structure(s)
*******************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	6000000/2,	/* 3 MHz */
	{ 50, 50 }
};

static MACHINE_DRIVER_START( ninjakid )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000) /* 3.00MHz */
	MDRV_CPU_MEMORY(ninjakid_primary_readmem,ninjakid_primary_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000) /* 3.00MHz */
	MDRV_CPU_MEMORY(ninjakid_secondary_readmem,ninjakid_secondary_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4) /* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame */

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, (32-4)*8-1 )
	MDRV_GFXDECODE(ninjakid_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(ninjakid)
	MDRV_VIDEO_UPDATE(ninjakid)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/*******************************************************************************
 Rom Definitions
*******************************************************************************/

ROM_START( ninjakun ) /* Original Board? */
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Main CPU */
	ROM_LOAD( "ninja-1.7a",  0x0000, 0x02000, CRC(1c1dc141) SHA1(423d3ed35e73a8d5bfce075a889b0322b207bd0d) )
	ROM_LOAD( "ninja-2.7b",  0x2000, 0x02000, CRC(39cc7d37) SHA1(7f0d0e1e92cb6a57f15eb7fc51a67112f1c5fc8e) )
	ROM_LOAD( "ninja-3.7d",  0x4000, 0x02000, CRC(d542bfe3) SHA1(3814d8f5b1acda21438fff4f71670fa653dc7b30) )
	ROM_LOAD( "ninja-4.7e",  0x6000, 0x02000, CRC(a57385c6) SHA1(77925a281e64889bfe967c3d42a388529aaf7eb6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Secondary CPU */
	ROM_LOAD( "ninja-5.7h",  0x0000, 0x02000, CRC(164a42c4) SHA1(16b434b33b76b878514f67c23315d4c6da7bfc9e) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE ) /* Graphics */
	ROM_LOAD16_BYTE( "ninja-6.7n",  0x0000, 0x02000, CRC(a74c4297) SHA1(87184d14c67331f2c8a2412e28f31427eddae799) )
	ROM_LOAD16_BYTE( "ninja-7.7p",  0x0001, 0x02000, CRC(53a72039) SHA1(d77d608ce9388a8956831369badd88a8eda8e102) )
	ROM_LOAD16_BYTE( "ninja-8.7s",  0x4000, 0x02000, CRC(4a99d857) SHA1(6aadb6a5c721a161a5c1bef5569c1e323e380cff) )
	ROM_LOAD16_BYTE( "ninja-9.7t",  0x4001, 0x02000, CRC(dede49e4) SHA1(8ce4bc02ec583b3885ca63fb5e2d5dad185fe192) )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE ) /* Graphics */
	ROM_LOAD16_BYTE( "ninja-10.2c", 0x0000, 0x02000, CRC(0d55664a) SHA1(955a607b4401ce9f3f807d53833a766152b0ef9b) )
	ROM_LOAD16_BYTE( "ninja-11.2d", 0x0001, 0x02000, CRC(12ff9597) SHA1(10b572844ab32e3ae54abe3600fecc1a811ac713) )
	ROM_LOAD16_BYTE( "ninja-12.4c", 0x4000, 0x02000, CRC(e9b75807) SHA1(cf4c8ac962f785e9de5502df58eab9b3725aaa28) )
	ROM_LOAD16_BYTE( "ninja-13.4d", 0x4001, 0x02000, CRC(1760ed2c) SHA1(ee4c8efcce483c8051873714856824a1a1e14b61) )
ROM_END

/*******************************************************************************
 Input Ports
********************************************************************************
 2 Sets of Controls
 2 Sets of Dipsiwtches
*******************************************************************************/

INPUT_PORTS_START( ninjakid )
	PORT_START	/* 0xa000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT | IPF_2WAY ) /* "XPOS1" */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT| IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_START1  )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 0xa001 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL) /* "YPOS1" */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT| IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_START2  )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* 0xa002 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x06, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x08, 0x08, "First Bonus" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x30, 0x30, "Second Bonus" )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_DIPSETTING(    0x10, "Every 30000" )
	PORT_DIPSETTING(    0x30, "Every 50000" )
	PORT_DIPSETTING(    0x20, "Every 70000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard"   )

	PORT_START /* DSW2 */
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
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )/* Probably Unused */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Endless Game (If Free Play)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*******************************************************************************
 Init
*******************************************************************************/

static DRIVER_INIT( ninjakid )
{
	/* Save State Stuff */
	state_save_register_UINT8 ("NK_Main", 0, "ninjakun_io_a002_ctrl", &ninjakun_io_a002_ctrl, 1);
}

/*******************************************************************************
 Game Drivers
*******************************************************************************/

GAME( 1984, ninjakun, 0, ninjakid, ninjakid, ninjakid, ROT0, "[UPL] (Taito license)", "Ninjakun Majou no Bouken" )
