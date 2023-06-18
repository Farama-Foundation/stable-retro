/*****************************************************************************

Momoko 120% (c) 1986 Jaleco

	Driver by Uki

	02/Mar/2001 -

******************************************************************************

Notes

Real machine has some bugs.(escalator bug, sprite garbage)
It is not emulation bug.
Flipped screen looks wrong, but it is correct.

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern data8_t *momoko_bg_scrollx;
extern data8_t *momoko_bg_scrolly;

VIDEO_UPDATE( momoko );

WRITE_HANDLER( momoko_fg_scrollx_w );
WRITE_HANDLER( momoko_fg_scrolly_w );
WRITE_HANDLER( momoko_text_scrolly_w );
WRITE_HANDLER( momoko_text_mode_w );
WRITE_HANDLER( momoko_bg_scrollx_w );
WRITE_HANDLER( momoko_bg_scrolly_w );
WRITE_HANDLER( momoko_flipscreen_w );
WRITE_HANDLER( momoko_fg_select_w);
WRITE_HANDLER( momoko_bg_select_w);
WRITE_HANDLER( momoko_bg_priority_w);

WRITE_HANDLER( momoko_bg_read_bank_w )
{
	data8_t *BG_MAP = memory_region(REGION_USER1);
	int bank_address = (data & 0x1f) * 0x1000;
	cpu_setbank(1, &BG_MAP[bank_address]);
}

/****************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },

	{ 0xd064, 0xd0ff, MRA_RAM }, /* sprite ram */

	{ 0xd400, 0xd400, input_port_0_r },
	{ 0xd402, 0xd402, input_port_1_r },
	{ 0xd406, 0xd406, input_port_2_r },
	{ 0xd407, 0xd407, input_port_3_r },

	{ 0xd800, 0xdbff, paletteram_r },
	{ 0xe000, 0xe3ff, MRA_RAM }, /* text */

	{ 0xf000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },

	{ 0xd064, 0xd0ff, MWA_RAM, &spriteram, &spriteram_size },

	{ 0xd400, 0xd400, MWA_NOP }, /* interrupt ack? */
	{ 0xd402, 0xd402, momoko_flipscreen_w },
	{ 0xd404, 0xd404, watchdog_reset_w },
	{ 0xd406, 0xd406, soundlatch_w },

	{ 0xd800, 0xdbff, paletteram_xxxxRRRRGGGGBBBB_swap_w, &paletteram },

	{ 0xdc00, 0xdc00, momoko_fg_scrolly_w },
	{ 0xdc01, 0xdc01, momoko_fg_scrollx_w },
	{ 0xdc02, 0xdc02, momoko_fg_select_w },

	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },

	{ 0xe800, 0xe800, momoko_text_scrolly_w },
	{ 0xe801, 0xe801, momoko_text_mode_w },

	{ 0xf000, 0xf001, momoko_bg_scrolly_w, &momoko_bg_scrolly },
	{ 0xf002, 0xf003, momoko_bg_scrollx_w, &momoko_bg_scrollx },
	{ 0xf004, 0xf004, momoko_bg_read_bank_w },
	{ 0xf006, 0xf006, momoko_bg_select_w },
	{ 0xf007, 0xf007, momoko_bg_priority_w },
MEMORY_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM2203_status_port_0_r },
	{ 0xa001, 0xa001, YM2203_read_port_0_r },
	{ 0xc000, 0xc000, YM2203_status_port_1_r },
	{ 0xc001, 0xc001, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, MWA_NOP }, /* unknown */
	{ 0xa000, 0xa000, YM2203_control_port_0_w },
	{ 0xa001, 0xa001, YM2203_write_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP }, /* unknown */
	{ 0xc000, 0xc000, YM2203_control_port_1_w },
	{ 0xc001, 0xc001, YM2203_write_port_1_w },
MEMORY_END

/****************************************************************************/

INPUT_PORTS_START( momoko )
    PORT_START
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

    PORT_START
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )

    PORT_START  /* dsw0 */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x03, "3" )
    PORT_DIPSETTING(    0x02, "4" )
    PORT_DIPSETTING(    0x01, "5" )
    PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
    PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coinage ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x04, DEF_STR( 2C_5C ) )
    PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
    PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x40, "Easy" )
    PORT_DIPSETTING(    0x60, "Normal" )
    PORT_DIPSETTING(    0x20, "Difficult" )
    PORT_DIPSETTING(    0x00, "Very difficult" )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START  /* dsw1 */
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life) )
    PORT_DIPSETTING(    0x01, "20000" )
    PORT_DIPSETTING(    0x03, "30000" )
    PORT_DIPSETTING(    0x02, "50000" )
    PORT_DIPSETTING(    0x00, "100000" )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x20, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )

    PORT_START	/* fake */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(	0x01, DEF_STR( On ) )

INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{4, 0},
	{0, 1, 2, 3, 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};

static struct GfxLayout spritelayout =
{
	8,16,     /* 8*16 characters */
	2048-128, /* 1024 sprites ( ccc 0ccccccc ) */
	4,        /* 4 bits per pixel */
	{12,8,4,0},
	{0, 1, 2, 3, 4096*8+0, 4096*8+1, 4096*8+2, 4096*8+3},
	{0, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16},
	8*32
};

static struct GfxLayout tilelayout =
{
	8,8,      /* 8*8 characters */
	8192-256, /* 4096 tiles ( cccc0 cccccccc ) */
	4,        /* 4 bits per pixel */
	{4,0,12,8},
	{0, 1, 2, 3, 4096*8+0, 4096*8+1, 4096*8+2, 4096*8+3},
	{0, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16},
	8*16
};

static struct GfxLayout charlayout1 =
{
	8,1,    /* 8*1 characters */
	256*8,  /* 2048 characters */
	2,      /* 2 bits per pixel */
	{4, 0},
	{0, 1, 2, 3, 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3},
	{8*0},
	8*1
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout1,      0,  24 }, /* TEXT */
	{ REGION_GFX2, 0x0000, &tilelayout,     256,  16 }, /* BG */
	{ REGION_GFX3, 0x0000, &charlayout,       0,   1 }, /* FG */
	{ REGION_GFX4, 0x0000, &spritelayout,   128,   8 }, /* sprite */
	{ -1 } /* end of array */
};

/****************************************************************************/

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	1250000,    /* 1.25 MHz */
	{ YM2203_VOL(40,15), YM2203_VOL(40,15) },
	{ 0, soundlatch_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( momoko )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)	/* 5.0MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 2500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 2.5MHz */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 29*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(momoko)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

/****************************************************************************/

ROM_START( momoko )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "momoko03.bin", 0x0000,  0x8000, CRC(386e26ed) SHA1(ad746ed1b87bafc5b4df9a28aade58cf894f4e7b) )
	ROM_LOAD( "momoko02.bin", 0x8000,  0x4000, CRC(4255e351) SHA1(27a0e8d8aea223d2128139582e3b66106f3608ef) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "momoko01.bin", 0x0000,  0x8000, CRC(e8a6673c) SHA1(f8984b063929305c9058801202405e6d45254b5b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE ) /* text */
	ROM_LOAD( "momoko13.bin", 0x0000,  0x2000, CRC(2745cf5a) SHA1(3db7c6319cac63df1620ef25508c5c45eaa4b141) )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE ) /* FG */
	ROM_LOAD( "momoko14.bin", 0x0000,  0x2000, CRC(cfccca05) SHA1(4ecff488a37ac76ecb9ecf8980bea30dcc9c9951) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD16_BYTE( "momoko16.bin", 0x0000,  0x8000, CRC(fc6876fc) SHA1(b2d06bc01ef9f4db9bf8902d67f31ccbb0fea61a) )
	ROM_LOAD16_BYTE( "momoko17.bin", 0x0001,  0x8000, CRC(45dc0247) SHA1(1b2bd4197ab7d237966e037c249b5bd623646c0b) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG */
	ROM_LOAD16_BYTE( "momoko09.bin", 0x00000, 0x8000, CRC(9f5847c7) SHA1(6bc9a00622d8a23446294a8d5d467375c5719125) )
	ROM_LOAD16_BYTE( "momoko11.bin", 0x00001, 0x8000, CRC(9c9fbd43) SHA1(7adfd7ea3dd6745c14e719883f1a86e0a3b3c0ff) )
	ROM_LOAD16_BYTE( "momoko10.bin", 0x10000, 0x8000, CRC(ae17e74b) SHA1(f52657ea6b6ac518b70fd7b811d9699da27f67d9) )
	ROM_LOAD16_BYTE( "momoko12.bin", 0x10001, 0x8000, CRC(1e29c9c4) SHA1(d78f102cefc9852b529dd317a76c7003ec2ad3d5) )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* BG map */
	ROM_LOAD( "momoko04.bin", 0x0000,  0x8000, CRC(3ab3c2c3) SHA1(d4a0d7f83bf64769e90a2c264c6114ac308cb8b5) )
	ROM_LOAD( "momoko05.bin", 0x8000,  0x8000, CRC(757cdd2b) SHA1(3471b42dc6458a18894dbd0638f4fe43c86dd70d) )
	ROM_LOAD( "momoko06.bin", 0x10000, 0x8000, CRC(20cacf8b) SHA1(e2b39abfc960e1c472e2bcf0cf06825c39941c03) )
	ROM_LOAD( "momoko07.bin", 0x18000, 0x8000, CRC(b94b38db) SHA1(9c9e45bbeca7b6b8b0051b144fb31fceaf5d6906) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* BG color/priority table */
	ROM_LOAD( "momoko08.bin", 0x0000,  0x2000, CRC(69b41702) SHA1(21b33b243dd6eaec8d41d9fd4d9e7faf2bd7f4d2) )

	ROM_REGION( 0x4000, REGION_USER3, 0 ) /* FG map */
	ROM_LOAD( "momoko15.bin", 0x0000,  0x4000, CRC(8028f806) SHA1(c7450d48803082f64af67fe752b6f49b71b6ff48) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 ) /* TEXT color */
	ROM_LOAD( "momoko-c.bin", 0x0000,  0x0100, CRC(f35ccae0) SHA1(60b99dd3c96637dacba7e96a143b1a2d6ffd28b9) )
	ROM_LOAD( "momoko-b.bin", 0x0100,  0x0020, CRC(427b0e5c) SHA1(aa2797b899571527cc96013fd3420b841954ee67) )
ROM_END

GAME( 1986, momoko, 0, momoko, momoko, 0, ROT0, "Jaleco", "Momoko 120%" )
