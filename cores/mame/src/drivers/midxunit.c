/*************************************************************************

	Midway X-unit system

	driver by Aaron Giles
	based on older drivers by Ernesto Corvi, Alex Pasadyn, Zsolt Vasvari

	Games supported:
		* Revolution X

	Known bugs:
		* none at this time

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "sndhrdw/dcs.h"
#include "midwunit.h"

#include "bootstrap.h"
#include "inptport.h"



/*************************************
 *
 *	Memory maps
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), midtunit_vram_data_r },
	{ TOBYTE(0x00800000), TOBYTE(0x00bfffff), midtunit_vram_color_r },
	{ TOBYTE(0x20000000), TOBYTE(0x20ffffff), MRA16_RAM },
	{ TOBYTE(0x60400000), TOBYTE(0x6040001f), midxunit_status_r },
	{ TOBYTE(0x60c00000), TOBYTE(0x60c0007f), midxunit_io_r },
	{ TOBYTE(0x60c000e0), TOBYTE(0x60c000ff), midwunit_security_r },
	{ TOBYTE(0x80800000), TOBYTE(0x8080001f), midxunit_analog_r },
	{ TOBYTE(0x80c00000), TOBYTE(0x80c000ff), midxunit_uart_r },
	{ TOBYTE(0xa0440000), TOBYTE(0xa047ffff), midwunit_cmos_r },
	{ TOBYTE(0xa0800000), TOBYTE(0xa08fffff), midxunit_paletteram_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_r },
	{ TOBYTE(0xc0c00000), TOBYTE(0xc0c000ff), midtunit_dma_r },
	{ TOBYTE(0xf8000000), TOBYTE(0xfeffffff), midwunit_gfxrom_r },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), midtunit_vram_data_w },
	{ TOBYTE(0x00800000), TOBYTE(0x00bfffff), midtunit_vram_color_w },
	{ TOBYTE(0x20000000), TOBYTE(0x20ffffff), MWA16_RAM, &midyunit_scratch_ram },
	{ TOBYTE(0x40800000), TOBYTE(0x4fffffff), midxunit_unknown_w },
	{ TOBYTE(0x60400000), TOBYTE(0x6040001f), midxunit_security_clock_w },
	{ TOBYTE(0x60c00080), TOBYTE(0x60c000df), midxunit_io_w },
	{ TOBYTE(0x60c000e0), TOBYTE(0x60c000ff), midxunit_security_w },
	{ TOBYTE(0x80800000), TOBYTE(0x8080001f), midxunit_analog_select_w },
	{ TOBYTE(0x80c00000), TOBYTE(0x80c000ff), midxunit_uart_w },
	{ TOBYTE(0xa0440000), TOBYTE(0xa047ffff), midxunit_cmos_w, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ TOBYTE(0xa0800000), TOBYTE(0xa08fffff), midxunit_paletteram_w, &paletteram16 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_w },
	{ TOBYTE(0xc0800000), TOBYTE(0xc08000ff), midtunit_dma_w },
	{ TOBYTE(0xc0c00000), TOBYTE(0xc0c000ff), midtunit_dma_w },
	{ TOBYTE(0xf8000000), TOBYTE(0xfbffffff), MWA16_ROM, (data16_t **)&midwunit_decode_memory },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MWA16_ROM, &midyunit_code_rom },
MEMORY_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( revx )
	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0f00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* coin door */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* bill validator */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0001, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0000, "Dipswitch Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0002, DEF_STR( On ))
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x001c, "1" )
	PORT_DIPSETTING(      0x0018, "2" )
	PORT_DIPSETTING(      0x0014, "3" )
	PORT_DIPSETTING(      0x000c, "ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x00e0, 0x00a0, "Credits" )
	PORT_DIPSETTING(      0x0020, "4 Start/2 Continue" )
	PORT_DIPSETTING(      0x00e0, "2 Start/2 Continue" )
	PORT_DIPSETTING(      0x00a0, "1 Start/1 Continue" )
	PORT_DIPSETTING(      0x0000, "3 Start/3 Continue" )
	PORT_DIPSETTING(      0x0040, "3 Start/2 Continue" )
	PORT_DIPSETTING(      0x0060, "4 Start/1 Continue" )
	PORT_DIPNAME( 0x0300, 0x0300, "Country" )
	PORT_DIPSETTING(      0x0300, "USA" )
	PORT_DIPSETTING(      0x0100, "French" )
	PORT_DIPSETTING(      0x0200, "German" )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( Unused ))*/
	PORT_DIPNAME( 0x0400, 0x0400, "Bill Validator" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0000, "Two Counters" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Players" )
	PORT_DIPSETTING(      0x1000, "3 Players" )
	PORT_DIPSETTING(      0x0000, "2 Players" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Cabinet ))
	PORT_DIPSETTING(      0x2000, "Rev X" )
	PORT_DIPSETTING(      0x0000, "Terminator 2" )
	PORT_DIPNAME( 0x4000, 0x4000, "Video Freeze" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER1, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER1, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER2, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER2, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER3, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER3, 39, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	34010 configuration
 *
 *************************************/

static struct tms34010_config cpu_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	midtunit_to_shiftreg,			/* write to shiftreg function */
	midtunit_from_shiftreg,			/* read from shiftreg function */
	0,								/* display address changed */
	0								/* display interrupt callback */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

/*
	visible areas and VBLANK timing based on these video params:

	          VERTICAL                   HORIZONTAL
	revx:     0014-0112 / 0120 (254)     0065-001F5 / 01F9 (400)
*/

static MACHINE_DRIVER_START( midxunit )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34020, 40000000/TMS34020_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(cpu_config)
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(MKLA5_FPS)
	MDRV_VBLANK_DURATION((1000000 * (288 - 254)) / (MKLA5_FPS * 288))
	MDRV_MACHINE_INIT(midxunit)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256)
	MDRV_VISIBLE_AREA(0, 399, 0, 253)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(midxunit)
	MDRV_VIDEO_UPDATE(midtunit)

	/* sound hardware */
	MDRV_IMPORT_FROM(dcs_audio_uart)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( revx )
	ROM_REGION( 0x10, REGION_CPU1, 0 )		/* 34020 dummy region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "revx_snd.2", ADSP2100_SIZE + 0x000000, 0x80000, CRC(4ed9e803) SHA1(ba50f1beb9f2a2cf5110897209b5e9a2951ff165) )
	ROM_LOAD( "revx_snd.3", ADSP2100_SIZE + 0x100000, 0x80000, CRC(af8f253b) SHA1(25a0000cab177378070f7a6e3c7378fe87fad63e) )
	ROM_LOAD( "revx_snd.4", ADSP2100_SIZE + 0x200000, 0x80000, CRC(3ccce59c) SHA1(e81a31d64c64e7b1d25f178c53da3d68453c203c) )
	ROM_LOAD( "revx_snd.5", ADSP2100_SIZE + 0x300000, 0x80000, CRC(a0438006) SHA1(560d216d21cb8073dbee0fd20ebe589932a9144e) )
	ROM_LOAD( "revx_snd.6", ADSP2100_SIZE + 0x400000, 0x80000, CRC(b7b34f60) SHA1(3b9682c6a00fa3bdb47e69d8e8ceccc244ee55b5) )
	ROM_LOAD( "revx_snd.7", ADSP2100_SIZE + 0x500000, 0x80000, CRC(6795fd88) SHA1(7c3790730a8b99b63112c851318b1c7e4989e5e0) )
	ROM_LOAD( "revx_snd.8", ADSP2100_SIZE + 0x600000, 0x80000, CRC(793a7eb5) SHA1(4b1f81b68f95cedf1b356ef362d1eb37acc74b16) )
	ROM_LOAD( "revx_snd.9", ADSP2100_SIZE + 0x700000, 0x80000, CRC(14ddbea1) SHA1(8dba9dc5529ea77c4312ea61f825bf9062ffc6c3) )

	ROM_REGION16_LE( 0x200000, REGION_USER1, ROMREGION_DISPOSE )	/* 34020 code */
	ROM_LOAD32_BYTE( "revx.51",  0x00000, 0x80000, CRC(9960ac7c) SHA1(441322f061d627ca7573f612f370a85794681d0f) )
	ROM_LOAD32_BYTE( "revx.52",  0x00001, 0x80000, CRC(fbf55510) SHA1(8a5b0004ed09391fe37f0f501b979903d6ae4868) )
	ROM_LOAD32_BYTE( "revx.53",  0x00002, 0x80000, CRC(a045b265) SHA1(b294d3a56e41f5ec4ab9bbcc0088833b1cab1879) )
	ROM_LOAD32_BYTE( "revx.54",  0x00003, 0x80000, CRC(24471269) SHA1(262345bd147402100785459af422dafd1c562787) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD( "revx.120", 0x0000000, 0x80000, CRC(523af1f0) SHA1(a67c0fd757e860fc1c1236945952a295b4d5df5a) )
	ROM_LOAD( "revx.121", 0x0080000, 0x80000, CRC(78201d93) SHA1(fb0b8f887eec433f7624f387d7fb6f633ea30d7c) )
	ROM_LOAD( "revx.122", 0x0100000, 0x80000, CRC(2cf36144) SHA1(22ed0eefa2c7c836811fac5f717c3f38254eabc2) )
	ROM_LOAD( "revx.123", 0x0180000, 0x80000, CRC(6912e1fb) SHA1(416f0de711d80e9182ede524c568c5095b1bec61) )

	ROM_LOAD( "revx.110", 0x0200000, 0x80000, CRC(e3f7f0af) SHA1(5877d9f488b0f4362a9482007c3ff7f4589a036f) )
	ROM_LOAD( "revx.111", 0x0280000, 0x80000, CRC(49fe1a69) SHA1(9ae54b461f0524c034fbcb6fcd3fd5ccb5d7265a) )
	ROM_LOAD( "revx.112", 0x0300000, 0x80000, CRC(7e3ba175) SHA1(dd2fe90988b544f67dbe6151282fd80d49631388) )
	ROM_LOAD( "revx.113", 0x0380000, 0x80000, CRC(c0817583) SHA1(2f866e5888e212b245984344950d0e1fb8957a73) )

	ROM_LOAD( "revx.101", 0x0400000, 0x80000, CRC(5a08272a) SHA1(17da3c9d71114f5fdbf50281a942be3da3b6f564) )
	ROM_LOAD( "revx.102", 0x0480000, 0x80000, CRC(11d567d2) SHA1(7ebe6fd39a0335e1fdda150d2dc86c3eaab17b2e) )
	ROM_LOAD( "revx.103", 0x0500000, 0x80000, CRC(d338e63b) SHA1(0a038217542667b3a01ecbcad824ee18c084f293) )
	ROM_LOAD( "revx.104", 0x0580000, 0x80000, CRC(f7b701ee) SHA1(0fc5886e5857326bee7272d5d482a878cbcea83c) )

	ROM_LOAD( "revx.91",  0x0600000, 0x80000, CRC(52a63713) SHA1(dcc0ff3596bd5d273a8d4fd33b0b9b9d588d8354) )
	ROM_LOAD( "revx.92",  0x0680000, 0x80000, CRC(fae3621b) SHA1(715d41ea789c0c724baa5bd90f6f0f06b9cb1c64) )
	ROM_LOAD( "revx.93",  0x0700000, 0x80000, CRC(7065cf95) SHA1(6c5888da099e51c4b1c592721c5027c899cf52e3) )
	ROM_LOAD( "revx.94",  0x0780000, 0x80000, CRC(600d5b98) SHA1(6aef98c91f87390c0759fe71a272a3ccadd71066) )

	ROM_LOAD( "revx.81",  0x0800000, 0x80000, CRC(729eacb1) SHA1(d130162ae22b99c84abfbe014c4e23e20afb757f) )
	ROM_LOAD( "revx.82",  0x0880000, 0x80000, CRC(19acb904) SHA1(516059b516bc5b1669c9eb085e0cdcdee520dff0) )
	ROM_LOAD( "revx.83",  0x0900000, 0x80000, CRC(0e223456) SHA1(1eedbd667f4a214533d1c22ca5312ecf2d4a3ab4) )
	ROM_LOAD( "revx.84",  0x0980000, 0x80000, CRC(d3de0192) SHA1(2d22c5bac07a7411f326691167c7c70eba4b371f) )

	ROM_LOAD( "revx.71",  0x0a00000, 0x80000, CRC(2b29fddb) SHA1(57b71e5c18b56bf58216e690fdefa6d30d88d34a) )
	ROM_LOAD( "revx.72",  0x0a80000, 0x80000, CRC(2680281b) SHA1(d1ae0701d20166a00d8733d9d12246c140a5fb96) )
	ROM_LOAD( "revx.73",  0x0b00000, 0x80000, CRC(420bde4d) SHA1(0f010cdeddb59631a5420dddfc142c50c2a1e65a) )
	ROM_LOAD( "revx.74",  0x0b80000, 0x80000, CRC(26627410) SHA1(a612121554549afff5c8e8c54774ca7b0220eda8) )

	ROM_LOAD( "revx.63",  0x0c00000, 0x80000, CRC(3066e3f3) SHA1(25548923db111bd6c6cff44bfb63cb9eb2ef0b53) )
	ROM_LOAD( "revx.64",  0x0c80000, 0x80000, CRC(c33f5309) SHA1(6bb333f563ea66c4c862ffd5fb91fb5e1b919fe8) )
	ROM_LOAD( "revx.65",  0x0d00000, 0x80000, CRC(6eee3e71) SHA1(0ef22732e0e2bb5207559decd43f90d1e338ad7b) )
	ROM_LOAD( "revx.66",  0x0d80000, 0x80000, CRC(b43d6fff) SHA1(87584e7aeea9d52a43023d40c359591ff6342e84) )

	ROM_LOAD( "revx.51",  0x0e00000, 0x80000, CRC(9960ac7c) SHA1(441322f061d627ca7573f612f370a85794681d0f) )
	ROM_LOAD( "revx.52",  0x0e80000, 0x80000, CRC(fbf55510) SHA1(8a5b0004ed09391fe37f0f501b979903d6ae4868) )
	ROM_LOAD( "revx.53",  0x0f00000, 0x80000, CRC(a045b265) SHA1(b294d3a56e41f5ec4ab9bbcc0088833b1cab1879) )
	ROM_LOAD( "revx.54",  0x0f80000, 0x80000, CRC(24471269) SHA1(262345bd147402100785459af422dafd1c562787) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEC( 1994, revx,   0,         midxunit, revx, revx, ROT0, "Midway",   "Revolution X (Rev. 1.0 6-16-94)", &revx_ctrl, &revx_bootstrap )
