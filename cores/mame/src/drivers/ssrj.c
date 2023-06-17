/***********************************
 Super Speed Race Jr (c) 1985 Taito
 driver by  Tomasz Slanina


 TODO:
 - colors (missing proms?)
 - dips
 - proper video hw emulation
 - controls (is there START button ?)

HW info :

	0000-7fff ROM
	c000-dfff VRAM ( 4 tilemaps (4 x $800) )
	e000-e7ff RAM
	e800-efff SCROLL RAM
	f003      ??
  f400-f401 AY 8910
  fc00      ??
  f800      ??

 Scroll RAM contains x and y offsets for each tileline,
 as well as other data (priroities ? additional flags ?)
 All moving obejcts (cars, etc) are displayed on tilemap 3.

 ------------------------------------
 Cheat :  $e210 - timer

************************************/

#include "driver.h"

extern unsigned char *ssrj_vram1,*ssrj_vram2,*ssrj_vram3,*ssrj_vram4,*ssrj_scrollram;

WRITE_HANDLER(ssrj_vram1_w);
WRITE_HANDLER(ssrj_vram2_w);
WRITE_HANDLER(ssrj_vram4_w);
READ_HANDLER(ssrj_vram1_r);
READ_HANDLER(ssrj_vram2_r);
READ_HANDLER(ssrj_vram4_r);

VIDEO_START( ssrj );
VIDEO_UPDATE( ssrj );
PALETTE_INIT( ssrj );

static int oldport=0x80;

MACHINE_INIT(ssrj)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	memset(&rom[0xc000],0,0x3fff); /* req for some control types */
	oldport=0x80;
}

static READ_HANDLER(ssrj_wheel_r)
{
	int port= input_port_1_r(0) -0x80;
	int retval=port-oldport;
	oldport=port;
	return retval;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, ssrj_vram1_r},
	{ 0xc800, 0xcfff, ssrj_vram2_r},
	{ 0xd000, 0xd7ff, MRA_RAM},
	{ 0xd800, 0xdfff, ssrj_vram4_r},
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xefff, MRA_RAM },
	{ 0xf000, 0xf000, input_port_0_r},
	{ 0xf001, 0xf001, ssrj_wheel_r },
	{ 0xf002, 0xf002, input_port_2_r},
	{ 0xf401, 0xf401 ,AY8910_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, ssrj_vram1_w,&ssrj_vram1 },
	{ 0xc800, 0xcfff, ssrj_vram2_w,&ssrj_vram2 },
	{ 0xd000, 0xd7ff, MWA_RAM,&ssrj_vram3 },
	{ 0xd800, 0xdfff, ssrj_vram4_w,&ssrj_vram4 },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, MWA_RAM,&ssrj_scrollram },
	{ 0xf003, 0xf003, MWA_NOP }, /* unknown */
	{ 0xf401, 0xf401, AY8910_write_port_0_w  },
	{ 0xf400, 0xf400, AY8910_control_port_0_w },
	{ 0xfc00, 0xfc00, MWA_NOP }, /* unknown */
	{ 0xf800, 0xf800, MWA_NOP }, /* wheel ? */
MEMORY_END

INPUT_PORTS_START( ssrj )

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_ANALOG( 0xe0, 0x00, IPT_PEDAL, 50, 0x20, 0, 0xe0 )

 PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_DIAL | IPF_REVERSE , 50, 4, 0x00, 0xff)


 PORT_START

 PORT_BIT( 0xf, IP_ACTIVE_LOW, IPT_BUTTON2  )  /* code @ $eef  , tested when controls = type4 */

 PORT_DIPNAME(0x30, 0x00, DEF_STR( Difficulty ) ) /* ??? code @ $62c */
 PORT_DIPSETTING(   0x10, "Easy" )
 PORT_DIPSETTING(   0x00, "Normal" )
 PORT_DIPSETTING(   0x20, "Difficult" )
 PORT_DIPSETTING(   0x30, "Very Difficult" )

 PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
 PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
 PORT_DIPSETTING(    0x00, DEF_STR( On ) )

 PORT_DIPNAME( 0x80, 0x80, "No Hit" )
 PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
 PORT_DIPSETTING(    0x00, DEF_STR( On ) )

 PORT_START

	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x030, 0x000, "Controls" ) /* 'press button to start' message, and wait for button2 */
	PORT_DIPSETTING(    0x00, "Type 1" )
	PORT_DIPSETTING(    0x10, "Type 2" )
	PORT_DIPSETTING(    0x20, "Type 3" )
	PORT_DIPSETTING(    0x30, "Type 4" )

	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* sometimes hangs after game over ($69b) */


INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,3),	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, RGN_FRAC(2,3), RGN_FRAC(1,3) },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 8*4 },
	{ -1 }	/* end of array */
};

static struct AY8910interface ay8910_interface =
{
	1,
	8000000/5,	/* guess */
	{ 30,},
	{ 0 }, /* not used ? */
	{input_port_3_r},
	{ 0 }, /* ? */
	{ 0 }
};


static MACHINE_DRIVER_START( ssrj )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 30*8-1, 3*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(ssrj)

	MDRV_VIDEO_START(ssrj)
	MDRV_VIDEO_UPDATE(ssrj)
	MDRV_ASPECT_RATIO(3,4)

	MDRV_MACHINE_INIT(ssrj)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ssrj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a40-01.bin",   0x0000, 0x4000, CRC(1ff7dbff) SHA1(a9e676ee087141d62f880cd98e7748db1e6e9461) )
	ROM_LOAD( "a40-02.bin",   0x4000, 0x4000, CRC(bbb36f9f) SHA1(9f85bac639d18ee932273a6c00b36ac969e69bb8) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a40-03.bin",   0x0000, 0x2000, CRC(3753182a) SHA1(3eda34f967563b11416344da87b7be46cbecff2b) )
	ROM_LOAD( "a40-04.bin",   0x2000, 0x2000, CRC(96471816) SHA1(e24b690085602b8bde079e596c2879deab128c83) )
	ROM_LOAD( "a40-05.bin",   0x4000, 0x2000, CRC(dce9169e) SHA1(2cdda1453b2913fad931788e1db0bc01ce923a04) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "proms",  0x0000, 0x0100, NO_DUMP )

ROM_END

GAMEX( 1985, ssrj,  0,       ssrj,  ssrj,  0, ORIENTATION_FLIP_X, "Taito Corporation", "Super Speed Race Junior (Japan)",GAME_WRONG_COLORS|GAME_IMPERFECT_GRAPHICS )
