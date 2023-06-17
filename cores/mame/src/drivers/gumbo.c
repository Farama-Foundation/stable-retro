/*

Gumbo (c)1994 Min Corp (Main Corp written on PCB)

argh it has the same music as news (news.c)

*/

/* working notes:

68k interrupts
lev 1 : 0x64 : 0000 0142 -
lev 2 : 0x68 : 0000 0142 -
lev 3 : 0x6c : 0000 0142 -
lev 4 : 0x70 : 0000 0142 -
lev 5 : 0x74 : 0000 0142 -
lev 6 : 0x78 : 0000 0142 -
lev 7 : 0x7c : 0000 0142 -

PCB Layout
----------

    M6295    U210     6264   U512
                      6264   U511
  ACTEL
  A1020A   14.31818MHz
DSW1          6116
              6116
                             6116
                             6116
  6264  6264
   U1    U2
   68000P10           U421   U420

*/

#include "driver.h"

data16_t *gumbo_bg_videoram;
data16_t *gumbo_fg_videoram;

WRITE16_HANDLER( gumbo_bg_videoram_w );
WRITE16_HANDLER( gumbo_fg_videoram_w );
VIDEO_START( gumbo );
VIDEO_UPDATE( gumbo );

static MEMORY_READ16_START( gumbo_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
	{ 0x1b0000, 0x1b03ff, MRA16_RAM },
	{ 0x1c0100, 0x1c0101, input_port_0_word_r },
	{ 0x1c0200, 0x1c0201, input_port_1_word_r },
	{ 0x1c0300, 0x1c0301, OKIM6295_status_0_lsb_r },
	{ 0x1e0000, 0x1e0fff, MRA16_RAM },
	{ 0x1f0000, 0x1f3fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( gumbo_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM }, /* main ram*/
	{ 0x1c0300, 0x1c0301, OKIM6295_data_0_lsb_w },
	{ 0x1b0000, 0x1b03ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0x1e0000, 0x1e0fff, gumbo_bg_videoram_w, &gumbo_bg_videoram }, /* bg tilemap*/
	{ 0x1f0000, 0x1f3fff, gumbo_fg_videoram_w, &gumbo_fg_videoram }, /* fg tilemap*/
MEMORY_END

INPUT_PORTS_START( gumbo )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Rotate" - also IPT_START1*/
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Help"*/
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Rotate" - also IPT_START2*/
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Help"*/
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Helps" )			/* "Power Count" in test mode*/
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0400, "1" )
	PORT_DIPNAME( 0x0800, 0x0800, "Bonus Bar Level" )
	PORT_DIPSETTING(      0x0800, "Normal" )
	PORT_DIPSETTING(      0x0000, "High" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Picture View" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout gumbo_layout =
{
	8,8,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,RGN_FRAC(1,2)+0, 8,RGN_FRAC(1,2)+8,  16,RGN_FRAC(1,2)+16,24,RGN_FRAC(1,2)+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout gumbo2_layout =
{
	4,4,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,RGN_FRAC(1,2)+0, 8,RGN_FRAC(1,2)+8 },
	{ 0*16, 1*16, 2*16, 3*16 },
	4*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gumbo_layout,   0x0, 2  }, /* bg tiles */
	{ REGION_GFX2, 0, &gumbo2_layout,  0x0, 2  }, /* fg tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

static MACHINE_DRIVER_START( gumbo )
	MDRV_CPU_ADD(M68000, 14318180 /2) /* or 10mhz? ?*/
	MDRV_CPU_MEMORY(gumbo_readmem,gumbo_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1) /* all the same*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(gumbo)
	MDRV_VIDEO_UPDATE(gumbo)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( gumbo )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u1.bin", 0x00001, 0x20000, CRC(e09899e4) SHA1(b62876dc3ada8509b766a80f496f1227b6af0ced) )
	ROM_LOAD16_BYTE( "u2.bin", 0x00000, 0x20000, CRC(60e59acb) SHA1(dd11329374c8f63851ddf5af54c91f78fad4fd3d) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u210.bin", 0x00000, 0x40000, CRC(16fbe06b) SHA1(4e40e62341dc886fcabdb07f64217dc086f43c67) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "u421.bin", 0x00000, 0x80000, CRC(42445132) SHA1(f29d09d040644c8ef12a1cfdfc0d066e8ed9b82d) )
	ROM_LOAD( "u420.bin", 0x80000, 0x80000, CRC(de1f0e2f) SHA1(3f46d19af48392794838a4b54f8c45b809c67d49) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* BG Tiles */
	ROM_LOAD( "u512.bin", 0x00000, 0x20000, CRC(97741798) SHA1(3603e14511817da19f6819d5612728d333695e99) )
	ROM_LOAD( "u511.bin", 0x20000, 0x20000, CRC(1411451b) SHA1(941d5f311f727e3a8d41ecbbe35b687d48cc2cef) )
ROM_END

GAME( 1994, gumbo, 0, gumbo, gumbo, 0, ROT0, "Min Corp.", "Gumbo" )
