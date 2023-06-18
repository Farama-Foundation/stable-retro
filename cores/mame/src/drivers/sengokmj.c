/******************************************************************************************

Sengoku Mahjong (c) 1991 Sigma

driver by Angelo Salese & Pierpaolo Prazzoli

Based on the D-Con driver.

TODO:
\-Video emulation:
 \-Find the scroll/tilemap enable registers(needed especially when you coin up at the intro).
 \-Positioning and colors needs fixing/double checking.

\-Other stuff:
 \-Missing NVRAM emulation.
   At startup a "Warning : Data in stock is wrong check ram" msg appears because of that.
 \-Understand what the uncommented port reads/writes really does...
 \-Merge the driver with the D-Con one,once that the V30 memory interface is changed to his
   natural behaviour(16-bit).

Notes:
\-Some strings written in the sound rom:
 \-"SENGOKU-MAHJONG Z80 PROGRAM ROM VERSION 1.00 WRITTEN BY K.SAEKI"
    at location 0x00c0-0x00ff.
 \-"Copyright 1990/1991 Sigma"
    at location 0x770-0x789.

\-To enter into various Service Mode items,press button F2,reset and then toggle it(i.e
  on->off).
\-To bypass the startup msg,toggle the "Reset" dip-sw or press F3.

******************************************************************************************/
/******************************************************************************************
Sengoku Mahjong (JPN Ver.)
(c)1991 Sigma

CPU:	uPD70116C-8 (V30)
Sound:	Z80-A
		YM3812
		M6295
OSC:	14.31818MHz
		16.000MHz
Chips:	SEI0100
		SEI0160
		SEI0200
		SEI0210
		SEI0220


MAH1-1-1.915  samples

MAH1-2-1.013  sound prg. (ic1013:27c512)

MM01-1-1.21   main prg.
MM01-2-1.24

RS006.89      video timing?

RSSENGO0.64   chr.
RSSENGO1.68

RSSENGO2.72   chr.


--- Team Japump!!! ---
http://www.rainemu.com/japump/
http://japump.i.am/
Dumped by Uki
10/26/2000

*******************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/seibu.h"

extern data8_t *bg_vram,*md_vram,*tx_vram,*fg_vram;
static UINT8 sengokumj_mux_data;

READ_HANDLER( sengoku_bg_vram_r );
READ_HANDLER( sengoku_fg_vram_r );
READ_HANDLER( sengoku_md_vram_r );
READ_HANDLER( sengoku_tx_vram_r );
WRITE_HANDLER( sengoku_bg_vram_w );
WRITE_HANDLER( sengoku_fg_vram_w );
WRITE_HANDLER( sengoku_md_vram_w );
WRITE_HANDLER( sengoku_tx_vram_w );
VIDEO_START( sengokmj );
VIDEO_UPDATE( sengokmj );

/*Multiplexer device for the mahjong panel*/
READ_HANDLER( mahjong_panel_0_r )
{
	switch(sengokumj_mux_data)
	{
		case 1:    return readinputport(3);
		case 2:    return readinputport(4);
		case 4:    return readinputport(5);
		case 8:    return readinputport(6);
		case 0x10: return readinputport(7);
		case 0x20: return readinputport(8);
	}
/*	usrintf_showmessage("Reading input port %02x at PC = %05x",sengokumj_mux_data,activecpu_get_pc());*/
	return readinputport(3);
}

READ_HANDLER( mahjong_panel_1_r )
{
	return readinputport(9);
}

WRITE_HANDLER( mahjong_panel_w )
{
	if(offset == 1)	{ sengokumj_mux_data = data; }
}

/***************************************************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites  */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tilelayout, 0x00*16,  0x40 }, /* Sprites */
	{ REGION_GFX2, 0x00000, &tilelayout, 0x00*16,  0x80 }, /* Tiles */
	{ REGION_GFX2, 0x00000, &charlayout, 0x00*16,  0x80 }, /* Text */
	{ -1 }
};

/***************************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x00000, 0x003ff, MRA_RAM },/*Just initialized at start-up,then not used at all...*/
	{ 0x06700, 0x068ff, MRA_RAM },
	{ 0x07800, 0x07fff, MRA_RAM },
	{ 0x08000, 0x087ff, MRA_RAM },
	{ 0x08800, 0x097ff, MRA_RAM },
	{ 0x09800, 0x09fff, MRA_RAM },
	{ 0x0c000, 0x0c7ff, sengoku_bg_vram_r },
	{ 0x0c800, 0x0cfff, sengoku_fg_vram_r },
	{ 0x0d000, 0x0d7ff, sengoku_md_vram_r },
	{ 0x0d800, 0x0e7ff, sengoku_tx_vram_r },
	{ 0x0f000, 0x0f7ff, paletteram_r },
	{ 0x0f800, 0x0ffff, MRA_RAM },
	{ 0xc0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0x003ff, MWA_RAM },  /*Ditto from above...*/
	{ 0x06700, 0x068ff, MWA_RAM },
	{ 0x07800, 0x07fff, MWA_RAM },
	{ 0x08000, 0x087ff, MWA_RAM },
	{ 0x08800, 0x097ff, MWA_RAM },
	{ 0x09800, 0x09fff, MWA_RAM },
	{ 0x0c000, 0x0c7ff, sengoku_bg_vram_w ,&bg_vram },
	{ 0x0c800, 0x0cfff, sengoku_fg_vram_w ,&fg_vram },
	{ 0x0d000, 0x0d7ff, sengoku_md_vram_w ,&md_vram },
	{ 0x0d800, 0x0e7ff, sengoku_tx_vram_w ,&tx_vram },
	{ 0x0e800, 0x0f7ff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0x0f800, 0x0ffff, MWA_RAM ,&spriteram },
	{ 0xc0000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x4000, 0x400f, seibu_main_v30_r },
	{ 0xc000, 0xc000, input_port_1_r },
	{ 0xc001, 0xc001, input_port_2_r },
	{ 0xc002, 0xc002, mahjong_panel_0_r },
	{ 0xc003, 0xc003, mahjong_panel_1_r },
	{ 0xc004, 0xc004, input_port_10_r },
	{ 0xc005, 0xc005, input_port_11_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x4000, 0x400f, seibu_main_v30_w },
	{ 0x8010, 0x801f, seibu_main_v30_w },
	{ 0x8140, 0x8141, mahjong_panel_w },
PORT_END

/***************************************************************************************/

SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);

/***************************************************************************************/

static INTERRUPT_GEN( sengokmj_interrupt )
{
	cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xcb/4);
}

/***************************************************************************************/
static MACHINE_DRIVER_START( sengokmj )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30, 16000000/2) /* V30-8 */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(sengokmj_interrupt,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(16*8, 56*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(sengokmj)
	MDRV_VIDEO_UPDATE(sengokmj)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

/***************************************************************************************/

INPUT_PORTS_START( sengokmj )
	/* Must be port 0: coin inputs read through sound cpu */
	SEIBU_COIN_INPUTS

/*$c000-$c001:Dip Switches.On most of them I have NO clue on what they really are */
/*they are checked at $d0107 afterwards btw*/
	PORT_START
	PORT_DIPNAME( 0x01,   0x00, DEF_STR( Demo_Sounds ) )/*$992e*/
	PORT_DIPSETTING(	  0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, "Re-start" )
	PORT_DIPSETTING(	  0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04,   0x04, "Double G" )/*$8850*/
	PORT_DIPSETTING(	  0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,   0x08, "Double L" )/*$8852*/
	PORT_DIPSETTING(	  0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x10, "Kamon" )/*$9930*/
	PORT_DIPSETTING(	  0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,   0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(	  0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,   0x40, "Out Sw" )/*$995a*/
	PORT_DIPSETTING(	  0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,   0x80, "Hopper" )/*$8932*/
	PORT_DIPSETTING(	  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x00, DEF_STR( On ) )
	PORT_START
	PORT_BIT(  0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*$c002 mux*/
	PORT_START/*w 1*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 A",   	KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 E",      KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 I",      KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 M",      KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 KAN",    KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 START",  KEYCODE_1,        IP_JOY_NONE )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START/*w 2*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,     	  IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 REACH", KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "P1 BET",   KEYCODE_2,         IP_JOY_NONE )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START/*w 4*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 C",     KEYCODE_C,     	  IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 G",     KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 K",     KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 CHI",   KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 RON",   KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START/*w 8*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 L",     KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 PON",   KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START/*w 0x10*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 LAST CHANCE", KEYCODE_RALT, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 TAKE SCORE", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 DOUBLE UP", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START/*w 0x20 : unused*/
	PORT_BIT(  0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*$c003 mux : unused*/
	PORT_START
	PORT_BIT(  0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START/*$c004 - $c005*/
	PORT_DIPNAME( 0x01, 0x01, "Door" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )/*"Service Mode"*/
	PORT_DIPNAME( 0x04, 0x04, "Opt. 1st" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Reset" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "unknown port 3-5" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Cash" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/*Used,causes "Hopper RunAway"msg if you toggle it.*/
	PORT_DIPNAME( 0x40, 0x40, "unknown port 3-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Meter" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_START
	PORT_BIT(  0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/***************************************************************************************/


ROM_START( sengokmj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* V30 code */
	ROM_LOAD16_BYTE( "mm01-1-1.21",  0xc0000, 0x20000, CRC(74076b46) SHA1(64b0ed5a8c32e21157ae12fe40519e4c605b329c))
	ROM_LOAD16_BYTE( "mm01-2-1.24",  0xc0001, 0x20000, CRC(f1a7c131) SHA1(d0fbbdedbff8f05da0e0296baa41369bc41a67e4))

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "mah1-2-1.013", 0x000000, 0x08000, CRC(6a4f31b8) SHA1(5e1d7ed299c1fd65c7a43faa02831220f4251733))
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rssengo2.72", 0x00000, 0x100000, CRC(fb215ff8) SHA1(f98c0a53ad9b97d209dd1f85c994fc17ec585bd7))

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rssengo0.64", 0x000000, 0x100000, CRC(36924b71) SHA1(814b2c69ab9876ccc57774e5718c05059ea23150))
	ROM_LOAD( "rssengo1.68", 0x100000, 0x100000, CRC(1bbd00e5) SHA1(86391323b8e0d3b7e09a5914d87fb2adc48e5af4))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "mah1-1-1.915", 0x00000, 0x20000, CRC(d4612e95) SHA1(937c5dbd25c89d4f4178b0bed510307020c5f40e))

	ROM_REGION( 0x200, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD( "rs006.89", 0x000, 0x200, CRC(96f7646e) SHA1(400a831b83d6ac4d2a46ef95b97b1ee237099e44)) /* Priority */
ROM_END

GAMEX( 1991, sengokmj, 0, sengokmj, sengokmj, 0,	ROT0, "Sigma", "Sengoku Mahjong (Japan)" ,GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
